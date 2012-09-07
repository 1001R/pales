package net.sf.pales.test;

import java.nio.file.FileSystems;
import java.util.UUID;

import net.sf.pales.PalesConfiguration;
import net.sf.pales.PalesLaunchRequest;
import net.sf.pales.PalesListener;
import net.sf.pales.PalesNotification;
import net.sf.pales.ProcessHandle;
import net.sf.pales.ProcessManager;

import org.junit.Test;

public class ProcessManagerTest {

	@Test
	public void test() {
		PalesConfiguration configuration = new PalesConfiguration();
		configuration.setDataDirectory(FileSystems.getDefault().getPath("C:/temp/pales/data"));
		configuration.setLauncher(FileSystems.getDefault().getPath("C:/temp/pales/bin/launch.exe"));
		configuration.setId("DEADBEEF");
		final ProcessManager procMgr = new ProcessManager();
		procMgr.init(configuration);
		
		PalesLaunchRequest request = new PalesLaunchRequest();
		request.setId(UUID.randomUUID().toString().replace("-", ""));
		request.setExecutable(FileSystems.getDefault().getPath("C:/temp/test.bat"));
		request.setStdoutFile(FileSystems.getDefault().getPath("C:/temp/out.txt"));
		request.setArgs(new String[] { "100" });
		request.setWorkingDirectory(FileSystems.getDefault().getPath("C:/Temp"));

//		final Semaphore semaphore = new Semaphore(0);
		
		procMgr.addListener(new PalesListener() {
			@Override
			public void notificationsAvailable() {
				PalesNotification n = procMgr.fetchNextNotification();
				System.out.println(n.getProcessHandle().getIrisId() + " " + n.getProcessStatus());
			}
		});
		
		ProcessHandle handle = procMgr.launch(request);
		try {
			Thread.sleep(10000);
		}
		catch (InterruptedException e) {
			System.out.println("sleep interrupted");
		}
		procMgr.cancelProcess(handle.getIrisId());
		
		try {
			Thread.sleep(60000);
		}
		catch (InterruptedException e) {
			
		}
	}

}
