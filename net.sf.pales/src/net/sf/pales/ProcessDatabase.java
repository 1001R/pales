package net.sf.pales;

import java.util.HashMap;
import java.util.Map;

public class ProcessDatabase {
	
	private Map<String, ProcessRecord> recordsById = new HashMap<>();
	
	
	public ProcessRecord getProcessRecord(String id) {
		return recordsById.get(id);
	}
	
	public ProcessRecord createProcessRecord(String processId) {
		ProcessRecord record = new ProcessRecord(processId);
		recordsById.put(processId, record);
		return record;
	}
	
	
}
