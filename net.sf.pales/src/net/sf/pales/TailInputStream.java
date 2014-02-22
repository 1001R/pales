package net.sf.pales;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.channels.SeekableByteChannel;
import java.nio.file.FileSystemException;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.nio.file.NoSuchFileException;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.nio.file.StandardWatchEventKinds;
import java.nio.file.WatchEvent;
import java.nio.file.WatchKey;
import java.nio.file.WatchService;
import java.nio.file.attribute.BasicFileAttributes;

public class TailInputStream extends InputStream {
	
	private static final int BUFFER_SIZE = 32 * 1024;
	private final Path path;
	private long filePosition = 0;
	private boolean fileDeleted = false;
	private boolean poll = false;
	
	private final ByteBuffer readBuffer;
	
	public TailInputStream(Path path) throws IOException {
		this(path, true);
	}
	
	public TailInputStream(Path path, boolean poll) throws IOException {
		this.path = path.toRealPath(LinkOption.NOFOLLOW_LINKS);
		readBuffer = ByteBuffer.allocate(BUFFER_SIZE);
		readBuffer.flip();
		this.poll = poll;
	}
	
	@Override
	public int read() throws IOException {
		throw new UnsupportedOperationException();
	}
	
	
	@Override
	public int read(byte[] b, int off, int len) throws IOException {
		if (len == 0) {
			return 0;
		}
		if (readBuffer.remaining() == 0 && fileDeleted) {
			return -1;
		}
		int n = Math.min(len, readBuffer.remaining());
		if (n > 0) {
			readBuffer.get(b, 0, n);
		}
		if (n == len) {
			return n;
		}
		try {
			readFromFile();
		} catch (InterruptedException ie) {
			
		}
		if (readBuffer.remaining() == 0 && fileDeleted && n == 0) {
			return -1;
		}
		off = n;
		len -= n;
		n = Math.min(len, readBuffer.remaining());
		if (n > 0) {
			readBuffer.get(b, 0, n);			
		}
		return off + n;
	}
	
	private void readFromFile() throws IOException, InterruptedException {
		readBuffer.clear();
		try {
			boolean done = false;
			while (!done) {
				try (SeekableByteChannel channel = Files.newByteChannel(path, StandardOpenOption.READ)) {
					channel.position(filePosition);
					int byteCount = channel.read(readBuffer);
					if (byteCount == -1) {
						channel.close();
						waitForFileToGrow();
					} else {
						filePosition += byteCount;
						done = true;
					}
				} catch (NoSuchFileException nsfe) {
					done = true;
					fileDeleted = true;
				} catch (FileSystemException fse) {
					Thread.sleep(1000);
				}
			}
		} finally {
			readBuffer.flip();
		}
	}
	
	private void pollForFileToGrow() throws IOException, InterruptedException {
		long fileSize = filePosition;
		while (fileSize <= filePosition) {
			Thread.sleep(1000);
			try {
				fileSize = getFileSize();
			} catch (IOException e) {
				return;
			}
		};
	}
	
	private void waitForFileToGrow() throws IOException, InterruptedException {
		if (poll) {
			pollForFileToGrow();
		} else {
			watchFile();
		}
	}
	
	
	private void watchFile() throws IOException, InterruptedException {
		WatchService watchService = FileSystems.getDefault().newWatchService();
		Path dirPath = path.getParent();
		WatchKey watchKey = dirPath.register(watchService, StandardWatchEventKinds.ENTRY_MODIFY, StandardWatchEventKinds.ENTRY_DELETE);
		try {
		while (true) {
			watchKey = watchService.take();
			for (WatchEvent<?> event: watchKey.pollEvents()) {
				boolean checkFileSize = false;
				if (event.kind() == StandardWatchEventKinds.OVERFLOW) {
					checkFileSize = true;
				} else {					
					Path file = ((WatchEvent<Path>) event).context();
					if (file.equals(path.getFileName())) {
						if (event.kind() == StandardWatchEventKinds.ENTRY_MODIFY) {
							checkFileSize = true;
						} else if (event.kind() == StandardWatchEventKinds.ENTRY_DELETE) {
							watchKey.cancel();
							return;
						}
					}
				}
				if (checkFileSize) {
					if (getFileSize() > filePosition) {
						watchKey.cancel();
						return;
					}					
				}
			}
			// reset the key
			if (!watchKey.reset()) {
				break;
			}
		}
		} finally {
			watchKey.cancel();
		}
	}
	
	private long getFileSize() throws IOException {
		BasicFileAttributes attrs = Files.readAttributes(path, BasicFileAttributes.class);
		return attrs.size();
	}
	
	public static void main(String[] args) throws IOException {
		TailInputStream cis = new TailInputStream(FileSystems.getDefault().getPath("C:/Temp/log.txt"), true);
		byte[] buf = new byte[4096];
		int byteCount;
		while ((byteCount = cis.read(buf)) > 0) {
			System.out.write(buf, 0, byteCount);
			System.out.flush();
		}
	}
}
