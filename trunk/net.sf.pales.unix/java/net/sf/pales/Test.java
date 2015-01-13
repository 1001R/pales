package net.sf.pales;

public class Test {

    public static void main(String[] args) throws Exception {
	//System.out.println(System.getProperty("java.library.path"));
	System.load("/home/improve/dev/net.sf.pales.jni.unix/libpales.so");
	final String procid = "DEADBEEF";
	System.out.println("Press key to launch process...");
	System.in.read();
	long pid = ProcessManager.launch(null, procid, 
	    "/home/improve/temp/pales",
	    "/home/improve/temp/pales/wd",
	    "/home/improve/temp/pales/stdout.txt",
	    "/home/improve/temp/pales/stderr.txt",
	    "/home/improve/temp/pales/script.sh",
	    new String[] { "X" });
	System.out.println("Launched process, pid = " + pid);
	/*
	try {
	   Thread.currentThread().sleep(30000L);
	} catch (InterruptedException e) {
	    e.printStackTrace();
	}
	System.out.println("Process running: " + OS.isProcessRunning(procid));
	OS.cancelProcess(procid);
	*/
    }

}
