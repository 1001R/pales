package net.sf.pales;

import java.nio.file.Path;

public class PalesConfiguration {
	private Path dataDirectory;
	private String id;

	public Path getDataDirectory() {
		return dataDirectory;
	}

	public void setDataDirectory(Path dataDirectory) {
		this.dataDirectory = dataDirectory;
	}
	
	public Path getDatabaseDirectory() {
		return getDataDirectory().resolve("db");
	}

	public String getId() {
		return id;
	}

	public void setId(String id) {
		this.id = id;
	}
}
