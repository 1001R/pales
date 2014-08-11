package net.sf.pales;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.logging.Level;

public class ProcessFile {
	private final Path filePath;
	private final String processId;
	private final ProcessStatus processStatus;
	private final boolean data;
	private long lastModifiedTime;
	
	public ProcessFile(Path filePath) {
		this.filePath = filePath;
		String fileName = filePath.getFileName().toString();
		int dotPos = fileName.lastIndexOf('.');
		if (dotPos < 0) {
			throw new IllegalArgumentException();
		}
		String extension = fileName.substring(dotPos + 1);
		processId = ProcessManager.decodePalesId(fileName.substring(1, dotPos));
		processStatus = ProcessStatus.fromAbbreviation(extension.charAt(0));
		data = fileName.charAt(0) == '1';
		
		try {
			lastModifiedTime = Files.getLastModifiedTime(filePath).toMillis();
		} catch (IOException e) {
			PalesLogger.SINGLETON.getLogger().log(Level.WARNING, "Cannot determine time of last modification of file " + filePath, e);
		}
	}

	public Path getFilePath() {
		return filePath;
	}

	public String getProcessId() {
		return processId;
	}

	public ProcessStatus getProcessStatus() {
		return processStatus;
	}

	public boolean isData() {
		return data;
	}

	public long getLastModifiedTime() {
		return lastModifiedTime;
	}
}
