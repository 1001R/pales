package net.sf.pales.test;

import static org.junit.Assert.fail;

import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.logging.Level;
import java.util.logging.Logger;

import net.sf.pales.PalesConfiguration;
import net.sf.pales.PalesLaunchRequest;
import net.sf.pales.PalesListener;
import net.sf.pales.ProcessManager;
import net.sf.pales.ProcessRecord;

import org.junit.Test;

public class ProcessManagerTest {
	
	public static final String PALES_WORKSPACE_DIRECTORY = "at.rufus.pales";
	
	private final PalesListener palesListener = new PalesListener() {
		@Override
		public void notificationsAvailable() {
			processPendingUpdates(false);
		}
	};
	
	private Logger log = Logger.getLogger(getClass().getName());
	private ProcessManager processManager;
	private final PalesConfiguration palesConfiguration;
	private static final Path DATA_DIRECTORY_PATH = Paths.get("C:/Temp/pales/data");
	private static final Path STDERR_FILE_PATH = Paths.get("C:/Temp/pales/stderr.txt");
	private static final Path STDOUT_FILE_PATH = Paths.get("C:/Temp/pales/stdout.txt");
	private static final Path WORKING_DIRECTORY_PATH = Paths.get("C:/Temp/pales/wd");
	private static final String PROCESS_MANAGER_ID = "8DC5E5B3-DB40-4558-87D0-207233C1E5F2";
	
	public ProcessManagerTest() {
		palesConfiguration = new PalesConfiguration();
		palesConfiguration.setDataDirectory(DATA_DIRECTORY_PATH);
		palesConfiguration.setId(PROCESS_MANAGER_ID);
	}
	
	private void start() throws IOException {
		processManager = new ProcessManager(palesConfiguration);
		log.log(Level.INFO, "ProcessManager has started.");
	}
	
	void startNotificationProcessing() {
		processManager.addListener(palesListener);
	}

	void stop() throws InterruptedException, IOException {
		processManager.removeListener(palesListener);
		processManager.shutdown();
		log.log(Level.INFO, "Shutdown of ProcessManager has been initiated..");
	}
	
	
	private void processPendingUpdates(boolean wait) {
		ProcessRecord processRecord;
		while ((processRecord = processManager.getNextUpdate(wait)) != null) {
			processNotification(processRecord);
		}
	}
	
	private void processNotification(ProcessRecord updatedProcessRecord) {
		try {
			primProcessNotification(updatedProcessRecord);	
		} catch (Throwable t) {
			log.log(Level.SEVERE, "Could not process notification: " + updatedProcessRecord, t);
		}
	}

	public void primProcessNotification(ProcessRecord updatedProcessRecord) throws Throwable {
		System.out.println("Update: " + updatedProcessRecord);
	}

	@Test
	public void test() throws Exception {
		start();
		PalesLaunchRequest request = new PalesLaunchRequest();
		request.setId("6BD91756-C81A-47AF-8F01-62004E0E8352");
		request.setExecutable(Paths.get("C:/Windows/notepad.exe"));
		request.setStderrFile(STDERR_FILE_PATH);
		request.setStdoutFile(STDOUT_FILE_PATH);
		request.setWorkingDirectory(WORKING_DIRECTORY_PATH);
		processManager.launch(request);
//		t.startNotificationProcessing();
		processPendingUpdates(true);
		fail("Not yet implemented");
	}

}
