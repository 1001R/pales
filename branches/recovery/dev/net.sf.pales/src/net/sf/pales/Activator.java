package net.sf.pales;

import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;

import org.apache.commons.lang3.SystemUtils;
import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;

public class Activator implements BundleActivator {

	private static BundleContext context;
	private static Path execHelperPath = null;

	static BundleContext getContext() {
		return context;
	}

	public void start(BundleContext bundleContext) throws Exception {
		Activator.context = bundleContext;
		System.loadLibrary("pales");
		if (SystemUtils.IS_OS_WINDOWS) {
			try (InputStream is = bundleContext.getBundle().getEntry("/launcher/execw.exe").openStream()) {
				Path tempFile = Files.createTempFile("pales", ".exe");
				tempFile.toFile().deleteOnExit();
				Files.copy(is, tempFile, StandardCopyOption.REPLACE_EXISTING);
				Activator.execHelperPath = tempFile;
			}
		}
	}

	public void stop(BundleContext bundleContext) throws Exception {
		Activator.context = null;
	}
	
	public static Path getExecHelperPath() {
		return execHelperPath;
	}

}
