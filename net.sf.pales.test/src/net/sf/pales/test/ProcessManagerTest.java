package net.sf.pales.test;

import java.nio.file.FileSystems;
import java.util.Timer;
import java.util.TimerTask;
import java.util.UUID;
import java.util.concurrent.Semaphore;

import net.sf.pales.OS;
import net.sf.pales.PalesConfiguration;
import net.sf.pales.PalesLaunchRequest;
import net.sf.pales.PalesListener;
import net.sf.pales.PalesNotification;
import net.sf.pales.ProcessHandle;
import net.sf.pales.ProcessManager;
import net.sf.pales.ProcessStatus;

import org.junit.Test;

public class ProcessManagerTest {

	@Test
	public void test() throws InterruptedException {
		PalesConfiguration configuration = new PalesConfiguration();
		configuration.setDataDirectory(FileSystems.getDefault().getPath("/home/phil/pales"));
//		configuration.setLauncher(FileSystems.getDefault().getPath("C:/temp/pales/bin/launch.exe"));
		configuration.setId("DEADBEEF");
		final ProcessManager procMgr = new ProcessManager();
		procMgr.init(configuration);
		
		PalesLaunchRequest request = new PalesLaunchRequest();
		request.setId(UUID.randomUUID().toString().replace("-", ""));
		request.setExecutable(FileSystems.getDefault().getPath("/home/phil/pales/tree.sh"));
		request.setStdoutFile(FileSystems.getDefault().getPath("/home/phil/pales/out.txt"));
		request.setStderrFile(FileSystems.getDefault().getPath("/home/phil/pales/err.txt"));
		request.setArgs(new String[] { "3", "3", "A" });
		request.setWorkingDirectory(FileSystems.getDefault().getPath("/tmp"));

		final Semaphore sem = new Semaphore(0); 
		
		procMgr.addListener(new PalesListener() {
			@Override
			public void notificationsAvailable() {
				sem.release();
			}
		});
		
		final ProcessHandle handle = procMgr.launch(request);
		Timer t = new Timer();
		t.schedule(new TimerTask() {
			@Override
			public void run() {
				System.out.println("Cancelling process");
				OS.cancelProcess(handle.getPalesId(), handle.getPid());
			}
		}, 10000);
		while (true) {
			sem.acquireUninterruptibly();
			PalesNotification n = procMgr.fetchNextNotification();
			System.out.println(n.getProcessHandle().getPalesId() + " " + n.getProcessStatus());
			if (n.getProcessStatus() == ProcessStatus.CANCELLED || n.getProcessStatus() == ProcessStatus.FINISHED) {
				break;
			}
		}
	}

}
