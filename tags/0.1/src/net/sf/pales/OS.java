package net.sf.pales;

public class OS {
	public static native String execute(String[] cmd);
	public static native void cancelProcess(String processId);
}
