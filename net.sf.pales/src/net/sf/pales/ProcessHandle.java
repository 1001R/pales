package net.sf.pales;


public class ProcessHandle implements Comparable<ProcessHandle> {
	private long pid;
	private final String irisId;
	
	ProcessHandle(String irisId) {
		this.irisId = irisId;
		this.pid = -1;
	}

	long getPid() {
		return pid;
	}

	void setPid(long pid) {
		this.pid = pid;
	}

	public String getIrisId() {
		return irisId;
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
			return getIrisId().equals(h.getIrisId());
		}
		return false;
	}
	
	@Override
	public int hashCode() {
		return getIrisId().hashCode();
	}

	@Override
	public int compareTo(ProcessHandle o) {
		return getIrisId().compareTo(o.getIrisId());
	}
}
