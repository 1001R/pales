package net.sf.pales;

import java.util.logging.Logger;

public enum PalesLogger {
	SINGLETON;
	
	private final Logger logger = Logger.getLogger(PalesLogger.class.getName());
	
	public Logger getLogger() {
		return logger;
	}
}
