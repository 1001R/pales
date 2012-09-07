package net.sf.pales;

import java.nio.file.Path;

public class PalesConfiguration {
	private Path dataDirectory;
	private Path launcher;
	private String id;

	public Path getDataDirectory() {
		return dataDirectory;
	}

	public void setDataDirectory(Path dataDirectory) {
		this.dataDirectory = dataDirectory;
	}

	public Path getLauncher() {
		return launcher;
	}

	public void setLauncher(Path launcher) {
		this.launcher = launcher;
	}

	public String getId() {
		return id;
	}

	public void setId(String id) {
		this.id = id;
	}
}
