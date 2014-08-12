package net.sf.pales;

import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.nio.channels.Channels;
import java.nio.channels.FileChannel;
import java.nio.charset.StandardCharsets;
import java.nio.file.ClosedWatchServiceException;
import java.nio.file.DirectoryStream;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.nio.file.StandardWatchEventKinds;
import java.nio.file.WatchEvent;
import java.nio.file.WatchKey;
import java.nio.file.WatchService;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.Timer;
import java.util.TimerTask;
import java.util.TreeMap;
import java.util.TreeSet;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.xml.bind.DatatypeConverter;

import org.apache.commons.lang3.SystemUtils;
import org.apache.commons.lang3.tuple.ImmutablePair;
import org.eclipse.core.runtime.ListenerList;

public class ProcessManager {
	
	private static final String DATA_PREFIX = "1";
	private static final String STATUS_PREFIX = "0";
	private static final long GRACE_PERIOD = 60000L;
	
	private final PalesConfiguration configuration;
	private TreeMap<ImmutablePair<Long, ProcessHandle>, PalesNotification> notifications = new TreeMap<>();
	private volatile boolean monitoring = false;
	private ListenerList listeners = new ListenerList();
	private ConcurrentHashMap<String, Long> processIdToPid = new ConcurrentHashMap<>();
	private static final Logger LOGGER = Logger.getLogger(ProcessManager.class.getName());
	private final Timer timer = new Timer("ProcessManager.timer");
	private ConcurrentHashMap<String, ProcessRecord> processRecords = new ConcurrentHashMap<>(16, 0.75f, 2);
	private BlockingQueue<ProcessFile> filesToProcess = new LinkedBlockingQueue<>();
	private BlockingQueue<String> modifiedProcesses = new LinkedBlockingQueue<>();
	private WatchService dbWatchService;
	
	private final Thread processFilesThread = new Thread("ProcessManager.processFiles") {
		@Override
		public void run() {
			processFiles();
		}
	};
	private final Thread monitorDatabaseThread = new Thread("ProcessManager.monitorDatabase") {
		@Override
		public void run() {
			monitorProcessDatabase();
		}
	};
	
	public ProcessManager(PalesConfiguration configuration) throws IOException {
		this.configuration = configuration;
		prepareDatabaseDirectory();
		startMonitoring();
		startup();
		startProcessFiles();
	}
	
	public void shutdown() throws InterruptedException, IOException {
		stopProcessFiles();
		stopMonitoring();
		filesToProcess.clear();
		modifiedProcesses.clear();
	}
	
	private void prepareDatabaseDirectory() {
		Path dbDirectory = configuration.getDatabaseDirectory();
		try {
			Files.createDirectories(dbDirectory);
		}
		catch (IOException e) {
			throw new RuntimeException("Database directory does not exist and cannot be created: " + dbDirectory);
		}
	}
	
	private WatchService registerDatabaseMonitor() throws IOException {
		WatchService watcher = FileSystems.getDefault().newWatchService();
		configuration.getDatabaseDirectory().register(watcher, StandardWatchEventKinds.ENTRY_CREATE);
		return watcher;
	}
	
	private void monitorProcessDatabase() {
		while (true) {
			WatchKey key = null;
			try {
				key = dbWatchService.take();
			}
			catch (InterruptedException e) {
				Thread.currentThread().interrupt();
				return;
			}
			catch (ClosedWatchServiceException e) {
				return;
			}
			if (key != null) {
				for (WatchEvent<?> event : key.pollEvents()) {
			        WatchEvent.Kind<?> kind = event.kind();
			        if (kind == StandardWatchEventKinds.OVERFLOW) {
			        	// TODO: we may have missed some event, so we should reinitialize
			            continue;
			        }
			        Path dir = (Path) key.watchable();
			        Path file = ((WatchEvent<Path>) event).context();
			        
			        try {
			        	ProcessFile processFile = new ProcessFile(dir.resolve(file));
			        	if (!processFile.isData()) {
			        		filesToProcess.add(processFile);
			        	}
			        } catch (Throwable t) {
			        	LOGGER.log(Level.WARNING, "Skipping file: " + file, t);
			        }
				}				        

			    boolean valid = key.reset();
			    if (!valid) {
		    		break;
			    }
			}
		}
	}
	
	private void startProcessFiles() {
		processFilesThread.start();
	}
	
	private void stopProcessFiles() throws InterruptedException {
		processFilesThread.interrupt();
		processFilesThread.join();
	}
	
	private void processFiles() {
		while (true) {
			try {
				ProcessFile file = filesToProcess.take();
				if (file.getProcessStatus() == ProcessStatus.FINAL || file.getProcessStatus() == ProcessStatus.DELETING) {
					continue;
				}
				ProcessRecord record = processRecords.get(file.getProcessId());
				if (record == null) {
					record = new ProcessRecord(file.getProcessId());
					updateProcessRecord(record, file);
					processRecords.putIfAbsent(record.getId(), record);
				} else {
					synchronized (record) {
						updateProcessRecord(record, file);
					}
				}
			} catch (InterruptedException e) {
				return;
			}
		}
	}
	
	private void updateProcessRecord(ProcessRecord record, ProcessFile file) {
		if (record.getStatus() == null || file.getProcessStatus().ordinal() > record.getStatus().ordinal()) {						
			record.setStatus(file.getProcessStatus());
			record.setLastMod(file.getLastModifiedTime());
			if (!record.isStale()) {							
				record.setStale(true);
				modifiedProcesses.add(record.getId());
				notifyListeners();
			}
		}
	}
	
	public ProcessRecord getProcessRecord(String processId) {
		ProcessRecord record = processRecords.get(processId);
		synchronized (record) {
			record.setStale(false);
			return record.clone();
		}
	}
	
	
	private void startMonitoring() throws IOException {
		dbWatchService = registerDatabaseMonitor();
		monitorDatabaseThread.start();
	}
	
	private void stopMonitoring() throws InterruptedException, IOException {
		try {
			dbWatchService.close();
		} catch (ClosedWatchServiceException e) {
			return;
		}
		monitorDatabaseThread.interrupt();
		monitorDatabaseThread.join();
	}
	
	private String getProcessId(Path path) {
		String fn = path.getFileName().toString();
		int i = fn.indexOf('-');
		if (i < 0) {
			return null;
		}
		return fn.substring(0, i);
	}
	
	private ProcessStatus getProcessStatus(Path path) {
		String fn = path.getFileName().toString();
		int i = fn.indexOf('-');
		if (i < 0) {
			return null;
		}
		return ProcessStatus.fromAbbreviation(fn.charAt(i + 1));
	}
	
	private int getProcessExitCode(Path path) {
		String fn = path.getFileName().toString();
		int i = fn.lastIndexOf('-');
		if (i < 0) {
			throw new IllegalArgumentException();
		}
		try {
			return Integer.parseInt(fn.substring(i + 1));
		} catch (NumberFormatException e) {
			throw new IllegalArgumentException();
		}
	}
	
	private long getProcessPid(Path path) {
		String fn = path.getFileName().toString();
		int i = fn.indexOf('-');
		if (i < 0) {
			return 0;
		}
		ProcessStatus status = ProcessStatus.fromAbbreviation(fn.charAt(i + 1));
		if (status != ProcessStatus.RUNNING) {
			return 0;
		}
		return Long.parseLong(fn.substring(i + 3));
	}
	
	private Object readData(Path dataFilePath, ProcessStatus processStatus) {
		if (processStatus == ProcessStatus.REQUESTED) {
			try (ObjectInputStream ois = new ObjectInputStream(Files.newInputStream(dataFilePath))) {
				return ois.readObject();
			} catch (ClassNotFoundException | IOException e) {
				throw new RuntimeException("Cannot read process launch request from file " + dataFilePath, e);
			}
		} else if (processStatus == ProcessStatus.RUNNING) {
			try {
				List<String> lines = Files.readAllLines(dataFilePath, StandardCharsets.US_ASCII);
				String s = lines.get(0);
				if (s.charAt(0) == '\uFEFF') {
					s = s.substring(1);
				}
				return new Integer(s);
			} catch (IOException e) {
				throw new RuntimeException("Cannot read PID from file " + dataFilePath, e);
			}
		}
		return null;
	}
	
	private void startup() {
		final long currentTime = System.currentTimeMillis();
		Set<Path> filePaths = new TreeSet<>();
		try (DirectoryStream<Path> stream = Files.newDirectoryStream(configuration.getDatabaseDirectory())) {
			for (Path p : stream) {
				filePaths.add(p);
			}
		}
		catch (IOException e) {
			throw new RuntimeException("Cannot traverse database directory " + configuration.getDatabaseDirectory(), e);
		}
		for (Path p : filePaths) {
			ProcessFile processFile = null;
			try {
				processFile = new ProcessFile(p);
			} catch (Throwable t) {
				LOGGER.log(Level.WARNING, "Invalid process file in database directory: " + p, t);
				tryDeleteFile(p);
				continue;
			}

			if (processFile.getProcessStatus() == ProcessStatus.FINAL) {
				continue;
			}
			
			if (processFile.isData()) {
				String dataFileName = p.getFileName().toString();
				String statusFileName = STATUS_PREFIX + dataFileName.substring(1);
				Path statusFilePath = p.getParent().resolve(statusFileName);
				if (!filePaths.contains(statusFilePath) && (currentTime - processFile.getLastModifiedTime()) > GRACE_PERIOD) {
					PalesLogger.SINGLETON.getLogger().log(Level.INFO, "Deleting data file from incomplete transaction", p);
					tryDeleteFile(p);
					continue;
				}
				ProcessRecord record = processRecords.get(processFile.getProcessId());
				if (record.getStatus() == processFile.getProcessStatus()) {
					record.setData(readData(p, record.getStatus()));
				}
			} else {
					ProcessRecord record = new ProcessRecord(processFile.getProcessId());
					record.setStatus(processFile.getProcessStatus());
					record.setLastMod(processFile.getLastModifiedTime());
					ProcessRecord oldRecord = processRecords.get(processFile.getProcessId());
					if (oldRecord == null || oldRecord.getStatus().compareTo(record.getStatus()) < 0) {
						processRecords.put(processFile.getProcessId(), record);
					}
			}
		}
		
		for (ProcessRecord record_ : processRecords.values()) {
			final ProcessRecord record = record_;
			if (record.getStatus() == ProcessStatus.DELETING) {
				processRecords.remove(record.getId());
				removeProcessFromDatabase(record.getId());
			}
			else if (record.getStatus() == ProcessStatus.REQUESTED) {
				long delay = GRACE_PERIOD - currentTime + record.getLastMod();
				if (delay < 0) {
					doLaunch((PalesLaunchRequest) record.getData());
				} else {
					final TimerTask task = new TimerTask() {
						@Override
						public void run() {
							synchronized (record) {
								if (record.getStatus() == ProcessStatus.REQUESTED) {
									doLaunch((PalesLaunchRequest) record.getData());
								}
							}
						}
					};
					timer.schedule(task, delay);
				}
			} else if (record.getStatus() == ProcessStatus.RUNNING) {
				if (!OS.isProcessRunning(encodePalesId(record.getId())) && !hasTerminated(record.getId())) {
					record.setStatus(ProcessStatus.ERROR);
					record.setData("Process or machine has crashed unexpectedly");
				}
			}
		}
		
		// debug only
		for (ProcessRecord record : processRecords.values()) {
			System.out.println(record);
		}
	}
	
	private boolean hasTerminated(String processId) {
		Path pathStatusFinal = configuration.getDatabaseDirectory().resolve(encodePalesId(processId) + '.' + ProcessStatus.FINAL.getAbbreviation());
		return Files.exists(pathStatusFinal);
	}
	
	private static void tryDeleteFile(Path filePath) {
		try {
			Files.delete(filePath);
		} catch (IOException e) {
			LOGGER.log(Level.WARNING, "Cannot delete file " + filePath, e);
		}
	}
	
	public ProcessHandle launch(PalesLaunchRequest request) {
		String encodedId = encodePalesId(request.getId());
		Path requestFilePath = configuration.getDatabaseDirectory().resolve(DATA_PREFIX + encodedId + '.' + ProcessStatus.REQUESTED.getAbbreviation());
		
		try (FileChannel requestFileChannel = FileChannel.open(requestFilePath, StandardOpenOption.CREATE_NEW, StandardOpenOption.WRITE);
				ObjectOutputStream oos = new ObjectOutputStream(Channels.newOutputStream(requestFileChannel))) {
			oos.writeObject(request);
			requestFileChannel.force(true);
		} catch (IOException e) {
			throw new RuntimeException("Cannot save request to file " + requestFilePath, e);
		}
		Path statusFilePath = configuration.getDatabaseDirectory().resolve(STATUS_PREFIX + encodedId + '.' + ProcessStatus.REQUESTED.getAbbreviation()); 
		try {
			createEmptyFile(statusFilePath);
		} catch (IOException e) {
			throw new RuntimeException("Cannot create process status file " + statusFilePath, e);
		}
		return doLaunch(request);
	}
	
	private static void createEmptyFile(Path filePath) throws IOException {
		try (FileChannel fileChannel = FileChannel.open(filePath, StandardOpenOption.CREATE_NEW, StandardOpenOption.WRITE)) {
			fileChannel.force(true);
		}
	}
	
	private ProcessHandle doLaunch(PalesLaunchRequest request) {
		String execHelper = SystemUtils.IS_OS_WINDOWS ? Activator.getExecHelperPath().toString() : null;
		long pid = launch(execHelper,
				encodePalesId(request.getId()),
				configuration.getDataDirectory().toString(),
				request.getWorkingDirectory().toString(),
				request.getStdoutFile() != null ? request.getStdoutFile().toString() : null,
				request.getStderrFile() != null ? request.getStderrFile().toString() : null,
				request.getExecutable().toString(),
				request.getArgs());
		processIdToPid.put(request.getId(), pid);
		return new ProcessHandle(request.getId(), pid);
		
	}
	
	private native static long launch(String execHelper, String processId, String palesDirectory, String workDirectory, String outFile, String errFile, String executable, String[] argv);
	
	static String encodePalesId(String palesId) {
		return DatatypeConverter.printBase64Binary(palesId.getBytes(StandardCharsets.UTF_8)).replace('/', '-');
	}
	
	static String decodePalesId(String encodedId) {
		return new String(DatatypeConverter.parseBase64Binary(encodedId.replace('-', '/')), StandardCharsets.UTF_8);
	}
	
	public void cancelProcess(String palesId) {
		OS.cancelProcess(encodePalesId(palesId));
	}
	
	public void addListener(PalesListener listener) {
		listeners.add(listener);
	}
	
	public void removeListener(PalesListener listener) {
		listeners.remove(listener);
	}
	
	private void notifyListeners() {
		Object[] l = listeners.getListeners();
		for (Object o : l) {
			PalesListener listener = (PalesListener) o;
			listener.notificationsAvailable();
		}
	}
	
	public PalesNotification fetchNextNotification() {
		Map.Entry<ImmutablePair<Long, ProcessHandle>, PalesNotification> firstEntry = null;
		synchronized (notifications) {
			firstEntry = notifications.pollFirstEntry();
			if (firstEntry != null) {
				PalesNotification notification = firstEntry.getValue();
				if (notification.getProcessStatus() == ProcessStatus.FINISHED || notification.getProcessStatus() == ProcessStatus.CANCELLED) {
					deleteProcess(notification.getProcessHandle().getPalesId());
				}
			}
		}
		return firstEntry != null ? firstEntry.getValue() : null;
	}
	
	public ProcessRecord getNextUpdate(boolean wait) {
		while (true) {
			String id = null;
			try { 
				id = wait ? modifiedProcesses.take() : modifiedProcesses.poll();
			} catch (InterruptedException e) {
				return null;
			}
			if (id == null) {
				return null;
			}
			ProcessRecord record = processRecords.get(id);
			synchronized (record) {
				if (record.isStale()) {
					record.setStale(false);
					return record.clone();
				}
			}
		}
	}
	
	private void deleteProcess(String processId) {
		List<Path> filesToDelete = new ArrayList<>();
		try (DirectoryStream<Path> ds = Files.newDirectoryStream(configuration.getDatabaseDirectory())) {
			for (Path p : ds) {
				if (p.getFileName().toString().startsWith(processId)) {
					filesToDelete.add(p);
				}
			}
		}
		catch (IOException e) {
			throw new RuntimeException(e);
		}
		for (Path p : filesToDelete) {
			try {
				deleteRetry(p);
			} catch (Exception e) {
				p.toFile().deleteOnExit();
			}
		}
	}
	
	private void deleteRetry(Path p) throws IOException, InterruptedException {
		long sleep = 2000L;
		int tries = 0;
		while (true) {
			try {
				Files.delete(p);
				return;
			} catch (IOException ioEx) {
				LOGGER.log(Level.WARNING, "Attempt " + (tries + 1) + " + to delete file \"" + p + "\" failed", ioEx);
				if (tries < 3) {
					tries++;
					try {
						Thread.sleep(sleep*tries);
					} catch (InterruptedException intEx) {
						throw intEx;
					}
				} else {
					throw ioEx;
				}
			}
		}
	}
	
	public boolean hasBeenLaunched(String processId) {
		synchronized (notifications) {
			for (PalesNotification notification : notifications.values()) {
				if (notification.getProcessHandle().getPalesId().equals(processId)) {
					return true;
				}
			}
		}
		return false;
	}
	
	public void removeProcess(String processId) throws IOException {
		ProcessRecord record = processRecords.get(processId);
		if (record == null) {
			return;
		}
		synchronized (record) {
			if (!record.getStatus().isFinal()) {
				return;
			}
			processRecords.remove(processId);
		}
		Path statusFilePath = configuration.getDatabaseDirectory().resolve(STATUS_PREFIX + encodePalesId(record.getId()) + '.' + ProcessStatus.DELETING.getAbbreviation());
		createEmptyFile(statusFilePath);
		removeProcessFromDatabase(processId);
	}
	
	private void removeProcessFromDatabase(String processId) {
		Path statusFilePath = null;
		try (DirectoryStream<Path> stream = Files.newDirectoryStream(configuration.getDatabaseDirectory())) {
			for (Path p : stream) {
				ProcessFile processFile = null;
				try {
					processFile = new ProcessFile(p);
					if (processFile.getProcessId().equals(processId)) {
						if (processFile.getProcessStatus() != ProcessStatus.DELETING) {
							Files.delete(p);
						} else {
							statusFilePath = p;
						}
						
					}
				} catch (Throwable t) {
					continue;
				}
			}
			if (statusFilePath != null) {
				Files.delete(statusFilePath);
			}
		}
		catch (IOException e) {
			throw new RuntimeException("Cannot delete process " + processId + " from database", e);
		}
	}
}