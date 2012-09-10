package net.sf.pales;


public class ProcessHandle implements Comparable<ProcessHandle> {
	private long pid;
	private final String palesId;
	
	ProcessHandle(String palesId) {
		this.palesId = palesId;
		this.pid = -1;
	}

	long getPid() {
		return pid;
	}

	void setPid(long pid) {
		this.pid = pid;
	}

	public String getPalesId() {
		return palesId;
	}
	
	@Override
	public boolean equals(Object obj) {
		if (obj == null) {
			return false;
		}
		if (obj == this) {
			return true;
		}
		if (obj instanceof ProcessHandle) {
			ProcessHandle h = (ProcessHandle) obj;
			return getPalesId().equals(h.getPalesId());
		}
		return false;
	}
	
	@Override
	public int hashCode() {
		return getPalesId().hashCode();
	}

	@Override
	public int compareTo(ProcessHandle o) {
		return getPalesId().compareTo(o.getPalesId());
	}
}
