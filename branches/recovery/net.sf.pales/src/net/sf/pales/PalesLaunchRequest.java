package net.sf.pales;

import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.nio.file.Path;
import java.nio.file.Paths;

public class PalesLaunchRequest implements Serializable {
	private static final long serialVersionUID = 1L;
	private transient Path executable;
	private String[] args = new String[0];
	private transient Path workingDirectory;
	private transient Path stdoutFile;
	private transient Path stderrFile;
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
	
	private void writeObject(ObjectOutputStream oos) throws IOException {
		oos.defaultWriteObject();
		writePathToObjectOutputStream(oos, executable);
		writePathToObjectOutputStream(oos, workingDirectory);
		writePathToObjectOutputStream(oos, stdoutFile);
		writePathToObjectOutputStream(oos, stderrFile);
	}
	
	private void readObject(ObjectInputStream ois) throws ClassNotFoundException, IOException {
		ois.defaultReadObject();
		setExecutable(readPathFromObjectInputStream(ois));
		setWorkingDirectory(readPathFromObjectInputStream(ois));
		setStdoutFile(readPathFromObjectInputStream(ois));
		setStderrFile(readPathFromObjectInputStream(ois));
	}
	
	private void writePathToObjectOutputStream(ObjectOutputStream oos, Path p) throws IOException {
		if (p == null) {
			oos.writeObject(null);
		} else {
			oos.writeObject(p.toString());
		}
	}
	
	private Path readPathFromObjectInputStream(ObjectInputStream ois) throws ClassNotFoundException, IOException {
		String s = (String) ois.readObject();
		return s != null ? Paths.get(s) : null;
	}
}
