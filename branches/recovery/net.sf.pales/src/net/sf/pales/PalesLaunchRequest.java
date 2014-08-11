package net.sf.pales;

import java.nio.file.Path;

public class PalesLaunchRequest {
	private Path executable;
	private String[] args;
	private Path workingDirectory;
	private Path stdoutFile;
	private Path stderrFile;
	private String id;
	
	public Path getExecutable() {
		return executable;
	}
	
	public void setExecutable(Path executable) {
		this.executable = executable;
	}
	
	public String[] getArgs() {
		return args;
	}
	
	public void setArgs(String[] args) {
		this.args = args;
	}
	
	public Path getWorkingDirectory() {
		return workingDirectory;
	}
	
	public void setWorkingDirectory(Path workingDirectory) {
		this.workingDirectory = workingDirectory;
	}
	
	public Path getStdoutFile() {
		return stdoutFile;
	}
	
	public void setStdoutFile(Path stdoutFile) {
		this.stdoutFile = stdoutFile;
	}
	
	public Path getStderrFile() {
		return stderrFile;
	}
	
	public void setStderrFile(Path stderrFile) {
		this.stderrFile = stderrFile;
	}
	
	public String getId() {
		return id;
	}
	
	public void setId(String id) {
		this.id = id;
	}
}
