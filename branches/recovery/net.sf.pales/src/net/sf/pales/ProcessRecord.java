package net.sf.pales;

import org.apache.commons.lang3.builder.ToStringBuilder;


public class ProcessRecord implements Comparable<ProcessRecord>, Cloneable {
	private final String id;
	private ProcessStatus status;
	private boolean committed;
	private Object data;
	private long lastMod;
	private boolean stale = false;
	private int exitCode = 0;
	
	ProcessRecord(String id) {
		this.id = id;
	}
	
	public String getId() {
		return id;
	}
	
	public ProcessStatus getStatus() {
		return status;
	}
	
	public void setStatus(ProcessStatus status) {
		this.status = status;
	}
	
	public boolean isCommitted() {
		return committed;
	}
	
	public void commit() {
		this.committed = true;
	}
	
	public void setData(Object data) {
		this.data = data;
	}
	
	public Object getData() {
		return data;
	}
	
	public void setLastMod(long lastMod) {
		this.lastMod = lastMod;
	}
	
	public long getLastMod() {
		return lastMod;
	}
	
	public int getExitCode() {
		return exitCode;
	}
	
	public void setExitCode(int exitCode) {
		this.exitCode = exitCode;
	}
	
	public void setStale(boolean stale) {
		this.stale = stale;
	}
	
	public boolean isStale() {
		return stale;
	}
	
	public ProcessRecord clone() {
		try {
			return (ProcessRecord) super.clone();
		} catch (CloneNotSupportedException e) {
			throw new RuntimeException(e);
		}
	}
	
	@Override
	public int compareTo(ProcessRecord o) {
		int result = id.compareTo(o.id);
		return result == 0 ? status.compareTo(o.status) : result;
	}
	
	@Override
	public String toString() {
		ToStringBuilder stringBuilder = new ToStringBuilder(this)
			.append("status", status);
		switch (getStatus()) {
		case REQUESTED:
			break;
		case RUNNING:
			break;
		case FINISHED:
			stringBuilder.append("exitCode", exitCode);
		case ERROR:
			stringBuilder.append("message", getData());
		}
		return stringBuilder.toString();
	}
}