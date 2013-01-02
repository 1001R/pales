package net.sf.pales.test;

import java.nio.file.FileSystems;
import java.util.UUID;

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
	public void test() {
		PalesConfiguration configuration = new PalesConfiguration();
		configuration.setDataDirectory(FileSystems.getDefault().getPath("C:/temp/pales/deadbeef"));
//		configuration.setLauncher(FileSystems.getDefault().getPath("C:/temp/pales/bin/launch.exe"));
		configuration.setId("DEADBEEF");
		final ProcessManager procMgr = new ProcessManager();
		procMgr.init(configuration);
		
		PalesLaunchRequest request = new PalesLaunchRequest();
		request.setId(UUID.randomUUID().toString().replace("-", ""));
		request.setExecutable(FileSystems.getDefault().getPath("C:/windows/notepad.exe"));
		request.setStdoutFile(FileSystems.getDefault().getPath("C:/temp/out.txt"));
		request.setArgs(new String[] { });
		request.setWorkingDirectory(FileSystems.getDefault().getPath("C:/Temp"));

		final Object x = new Object();
		
		procMgr.addListener(new PalesListener() {
			@Override
			public void notificationsAvailable() {
				synchronized (x) {
					x.notify();
				}
			}
		});
		
		ProcessHandle handle = procMgr.launch(request);
		while (true) {
			synchronized (x) {
				try {
					x.wait();
				}
				catch (InterruptedException e) {
					continue;
				}				
			}
			PalesNotification n = procMgr.fetchNextNotification();
			System.out.println(n.getProcessHandle().getPalesId() + " " + n.getProcessStatus());
			if (n.getProcessStatus() == ProcessStatus.CANCELLED || n.getProcessStatus() == ProcessStatus.FINISHED) {
				break;
			}
		}
	}

}
