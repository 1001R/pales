package net.sf.pales;

public class OS {
	public static native void cancelProcess(String processId, long pid);
	public static native boolean isProcessRunning(String processId);
}
