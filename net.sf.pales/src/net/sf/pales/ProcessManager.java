package net.sf.pales;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.DirectoryStream;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.nio.file.Path;
import java.nio.file.StandardWatchEventKinds;
import java.nio.file.WatchEvent;
import java.nio.file.WatchKey;
import java.nio.file.WatchService;
import java.nio.file.attribute.FileTime;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.TimeUnit;

import org.apache.commons.lang3.SystemUtils;
import org.apache.commons.lang3.tuple.ImmutablePair;
import org.eclipse.core.runtime.ListenerList;

public class ProcessManager {
	
	private PalesConfiguration configuration;
	private TreeMap<ImmutablePair<Long, ProcessHandle>, PalesNotification> notifications = new TreeMap<>();
	private Thread monitorThread;
	private volatile boolean monitoring = false;
	private ListenerList listeners = new ListenerList();
	private ConcurrentHashMap<String, Long> processIdToPid = new ConcurrentHashMap<>();
	
	public void init(PalesConfiguration configuration) {
		this.configuration = configuration;
		startMonitoring();
		List<Path> dbFiles = new ArrayList<>();
		try (DirectoryStream<Path> stream = Files.newDirectoryStream(configuration.getDatabaseDirectory())) {
			for (Path file : stream) {
				Path f = file.getFileName();
				dbFiles.add(f);
				handleFile(configuration.getDatabaseDirectory(), f);
			}
		}
		catch (IOException e) {
			stopMonitoring();
			throw new RuntimeException(e);
		}
	}
	
	private void monitorProcessDatabase() {
		try (WatchService watcher = FileSystems.getDefault().newWatchService()) {
			configuration.getDatabaseDirectory().register(watcher, StandardWatchEventKinds.ENTRY_CREATE);
			monitoring = true;
			synchronized (this) {
				notifyAll();
			}
			while (monitoring) {
				WatchKey key = null;
				try {
					key = watcher.take();
				}
				catch (InterruptedException e) {
					// ignore
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
				        handleFile(dir, file);
				    }

				    boolean valid = key.reset();
				    if (!valid) {
			    		break;
				    }
				}
			}
		}
		catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	private void startMonitoring() {
		monitorThread = new Thread(new Runnable() {
			@Override
			public void run() {
				monitorProcessDatabase();
			}
		});
		monitoring = false;
		monitorThread.start();
		synchronized (this) {
			while (!monitoring) {
				try {
					wait();
				}
				catch (InterruptedException e) {
					
				}
			}
		}
	}
	
	private void stopMonitoring() {
		monitoring = false;
		monitorThread.interrupt();
		try {
			monitorThread.join();
		}
		catch (InterruptedException e) {
			
		}
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
	
	private void handleFile(Path directory, Path file) {
		String id = getProcessId(file);
        ProcessStatus status = getProcessStatus(file);
        ProcessHandle handle = new ProcessHandle(id);
      
        Path fullPath = directory.resolve(file);
        long timestamp = -1;
        try {
        	FileTime mtime = Files.getLastModifiedTime(fullPath, LinkOption.NOFOLLOW_LINKS);
        	timestamp = mtime.to(TimeUnit.NANOSECONDS);
        }
        catch (IOException e) {
        	return;
        }
        
    	switch (status) {
    	case RUNNING:
    		markRunning(handle, timestamp);
    		break;
    	case FINISHED:
    		markFinished(handle, timestamp);
    		break;
    	case CANCELLED:
    		markCancelled(handle, timestamp);
    		break;
    	}

	}
	
	private void markRunning(ProcessHandle handle, long timestamp) {
		if (!processIdToPid.containsKey(handle.getPalesId())) {
			processIdToPid.put(handle.getPalesId(), handle.getPid());
		}
		updateNotification(handle, ProcessStatus.RUNNING, timestamp);
	}
	
	private void markFinished(ProcessHandle handle, long timestamp) {
		updateNotification(handle, ProcessStatus.FINISHED, timestamp);
	}
	
	private void markCancelled(ProcessHandle handle, long timestamp) {
		updateNotification(handle, ProcessStatus.CANCELLED, timestamp);
	}
	
	private void updateNotification(ProcessHandle handle, ProcessStatus status, long timestamp) {
		PalesNotification notification = new PalesNotification(handle, status, timestamp);
		ImmutablePair<Long, ProcessHandle> key = new ImmutablePair<>(timestamp, handle);
		synchronized (notifications) {
			if (!notifications.containsKey(key)) {
				notifications.put(key, notification);
				notifyListeners();
			}
		}
	}
	
	public ProcessHandle launch(PalesLaunchRequest request) {
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
	
	private static String encodePalesId(String palesId) {
		StringBuilder sb = new StringBuilder();
		palesId.getBytes(StandardCharsets.UTF_8);
		for (int i = 0; i < palesId.length(); i++) {
			char c = palesId.charAt(i);
			if ("abcdefghijklmnopqrstuvwxyz0123456789".indexOf(Character.toLowerCase(c)) < 0) {
				sb.append('_').append(String.format("%04x", palesId.codePointAt(i)));
			}
			else {
				sb.append(c);
			}
		}
		return sb.toString();
	}
	
	public void cancelProcess(String palesId) {
		long pid = processIdToPid.get(palesId);
		OS.cancelProcess(palesId, pid);
	}
	
	public void startup() {
		
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
			((PalesListener) o).notificationsAvailable();
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
				Files.delete(p);
			}
			catch (IOException e) {
				p.toFile().deleteOnExit();
			}
		}
	}
	
	public void shutdown() {
		stopMonitoring();
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
}
