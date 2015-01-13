package net.sf.pales;

public class ProcessManager {
    public native static long launch(String execHelper, String processId, String palesDirectory, String workDirectory, String outFile, String errFile, String executable, String[] argv);
}
