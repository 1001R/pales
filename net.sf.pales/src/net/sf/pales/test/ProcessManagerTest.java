package net.sf.pales.test;

import static org.junit.Assert.fail;

import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Timer;
import java.util.TimerTask;
import java.util.UUID;
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
	private static final Path PALES_BASE_DIR = Paths.get("C:/Temp/pales");
	private static final Path DATA_DIRECTORY_PATH = PALES_BASE_DIR.resolve("data");
	private static final Path WORKING_DIRECTORY_PATH = PALES_BASE_DIR.resolve("wd");
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
		if (updatedProcessRecord.getStatus().isFinal()) {
			System.out.println("Processing of process " + updatedProcessRecord.getId() + " is finished. Deleting it from database...");
			processManager.removeProcess(updatedProcessRecord.getId());
		}
	}

	@Test
	public void test() throws Exception {
		start();

		final PalesLaunchRequest notepadRequest = new PalesLaunchRequest();
		notepadRequest.setId(UUID.randomUUID().toString());
		notepadRequest.setExecutable(Paths.get("C:/Windows/notepad.exe"));
		notepadRequest.setStderrFile(PALES_BASE_DIR.resolve(notepadRequest.getId().toString() + ".err"));
		notepadRequest.setStdoutFile(PALES_BASE_DIR.resolve(notepadRequest.getId().toString() + ".out"));
		notepadRequest.setWorkingDirectory(WORKING_DIRECTORY_PATH);
		processManager.launch(notepadRequest);

		Timer t = new Timer();
		t.schedule(new TimerTask() {
			@Override
			public void run() {
				processManager.cancelProcess(notepadRequest.getId());
			}
		}, 10000);
		
		final PalesLaunchRequest calcRequest = new PalesLaunchRequest();
		calcRequest.setId(UUID.randomUUID().toString());
		calcRequest.setExecutable(Paths.get("C:/Windows/System32/calc.exe"));
		calcRequest.setStderrFile(PALES_BASE_DIR.resolve(calcRequest.getId().toString() + ".err"));
		calcRequest.setStdoutFile(PALES_BASE_DIR.resolve(calcRequest.getId().toString() + ".out"));
		calcRequest.setWorkingDirectory(WORKING_DIRECTORY_PATH);
		processManager.launch(calcRequest);

		processPendingUpdates(true);
		fail("Not yet implemented");
	}

}
