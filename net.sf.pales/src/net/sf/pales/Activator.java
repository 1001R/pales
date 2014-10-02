package net.sf.pales;

import java.net.URL;
import java.nio.file.Path;

import org.apache.commons.lang3.SystemUtils;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.URIUtil;
import org.eclipse.osgi.service.datalocation.Location;
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
			try {
				URL launcherURL = bundleContext.getBundle().getEntry("/launcher/execw.exe");
				if (launcherURL != null) {
					launcherURL = FileLocator.toFileURL(launcherURL);
					execHelperPath = URIUtil.toFile(URIUtil.toURI(launcherURL)).toPath();
				} else {
					Location installLocation = Platform.getInstallLocation();
					Path installLocationPath = URIUtil.toFile(URIUtil.toURI(installLocation.getURL())).toPath();
					execHelperPath = installLocationPath.resolve("execw.exe");
				}
			} catch (Throwable t) {
				throw new RuntimeException("Cannot find process launcher", t);
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
