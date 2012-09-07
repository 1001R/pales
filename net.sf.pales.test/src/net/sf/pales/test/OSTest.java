package net.sf.pales.test;

import net.sf.pales.OS;

import org.junit.Assert;
import org.junit.Test;

public class OSTest {

	@Test
	public void testExecute() {
		String s = OS.execute(new String[] { "c:\\temp\\pales\\bin\\launch.exe", "-i",  "deadbeef",  "-d", "c:\\temp\\pales\\data", "-w", "c:\\temp", "-o", "c:\\temp\\out.txt", "c:\\temp\\test.bat", "5" });
		System.out.println(s);
		Assert.assertTrue(s != null);
	}

}
