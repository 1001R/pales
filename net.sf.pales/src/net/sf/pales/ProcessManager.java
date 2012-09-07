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
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.concurrent.TimeUnit;

import org.apache.commons.lang3.tuple.ImmutablePair;
import org.eclipse.core.runtime.ListenerList;

public class ProcessManager {
	
	private PalesConfiguration configuration;
	private TreeMap<ImmutablePair<Long, ProcessHandle>, PalesNotification> notifications = new TreeMap<>();
	private Thread monitorThread;
	private volatile boolean monitoring = false;
	private ListenerList listeners = new ListenerList();
	
	
	public void init(PalesConfiguration configuration) {
		this.configuration = configuration;
		startMonitoring();
		try (DirectoryStream<Path> stream = Files.newDirectoryStream(configuration.getDataDirectory())) {
			for (Path file : stream) {
				Path f = file.getFileName();
				handleFile(configuration.getDataDirectory(), f);
			}
		}
		catch (IOException e) {
			stopMonitoring();
			throw new RuntimeException(e);
		}
	}
	
	private void monitorProcessDatabase() {
		try (WatchService watcher = FileSystems.getDefault().newWatchService()) {
			configuration.getDataDirectory().register(watcher, StandardWatchEventKinds.ENTRY_CREATE);
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
			throw new RuntimeException(e);
		}
	}
	
	private void startMonitoring() {
		monitoring = true;
		monitorThread = new Thread(new Runnable() {
			@Override
			public void run() {
				monitorProcessDatabase();
			}
		});
		monitorThread.start();
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
	
	private void handleFile(Path directory, Path file) {
		String fileName = file.toString();
		if (fileName.charAt(0) == '.') {
			return;
		}
		int pos = fileName.indexOf('-');
		if (pos < 0 || fileName.length() == pos + 1) {
			return;
		}
		String id = fileName.substring(0, pos);
        ProcessStatus status = ProcessStatus.fromAbbreviation(fileName.charAt(pos + 1));
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
		List<String> cmd = new ArrayList<>();
		cmd.add(configuration.getLauncher().toString());
		cmd.add("-i");
		cmd.add(encodeIrisId(request.getId()));
		cmd.add("-d");
		cmd.add(configuration.getDataDirectory().toString());
		
		if (request.getWorkingDirectory() != null) {
			cmd.add("-w");
			cmd.add(request.getWorkingDirectory().toString());
		}
		if (request.getStderrFile() != null) {
			cmd.add("-e");
			cmd.add(request.getStderrFile().toString());
		}
		if (request.getStdoutFile() != null) {
			cmd.add("-o");
			cmd.add(request.getStdoutFile().toString());
		}
		cmd.add(request.getExecutable().toString());
		cmd.addAll(Arrays.asList(request.getArgs()));
		OS.execute(cmd.toArray(new String[0]));
		return new ProcessHandle(request.getId());
	}
	
	private static String encodeIrisId(String irisId) {
		StringBuilder sb = new StringBuilder();
		irisId.getBytes(StandardCharsets.UTF_8);
		for (int i = 0; i < irisId.length(); i++) {
			char c = irisId.charAt(i);
			if ("abcdefghijklmnopqrstuvwxyz0123456789".indexOf(Character.toLowerCase(c)) < 0) {
				sb.append('_').append(String.format("%04x", irisId.codePointAt(i)));
			}
			else {
				sb.append(c);
			}
		}
		return sb.toString();
	}
	
	public void cancelProcess(String irisId) {
		OS.cancelProcess(irisId);
	}
	
	public void startup() {
		
	}
	
	public void addListener(PalesListener listener) {
		listeners.add(listener);
	}
	
	public void removeIrisListener(PalesListener listener) {
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
					Path dataDir = configuration.getDataDirectory();
					Path f = dataDir.resolve(dataDir.getFileSystem().getPath(notification.getProcessHandle().getIrisId() + "-" + notification.getProcessStatus().getAbbreviation()));
					try {
						Files.delete(f);
					}
					catch (IOException e) {
						throw new RuntimeException(e);
					}
				}
			}
		}
		return firstEntry != null ? firstEntry.getValue() : null;
	}
	
	public void shutdown() {
		stopMonitoring();
	}
}
