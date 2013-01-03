package net.sf.pales;

public enum ProcessStatus {
	RUNNING {
		@Override
		public char getAbbreviation() {
			return 'R';
		}
	},
	CANCELLED {
		@Override
		public char getAbbreviation() {
			return 'C';
		}
	},
	FINISHED {
		@Override
		public char getAbbreviation() {
			return 'F';
		}
	};
	
	public abstract char getAbbreviation();
	public static ProcessStatus fromAbbreviation(char abbreviation) {
		for (ProcessStatus s : values()) {
			if (s.getAbbreviation() == abbreviation) {
				return s;
			}
		}
		throw new IllegalArgumentException();
	}
}
