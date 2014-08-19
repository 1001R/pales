package net.sf.pales;

public enum ProcessStatus {
	REQUESTED {
		@Override
		public char getAbbreviation() {
			return 'Q';
		}

		@Override
		public boolean isFinal() {
			return false;
		}
	},
	RUNNING {
		@Override
		public char getAbbreviation() {
			return 'R';
		}

		@Override
		public boolean isFinal() {
			return false;
		}
	},
	CANCELLED {
		@Override
		public char getAbbreviation() {
			return 'C';
		}

		@Override
		public boolean isFinal() {
			return true;
		}
	},
	FINISHED {
		@Override
		public char getAbbreviation() {
			return 'F';
		}

		@Override
		public boolean isFinal() {
			return true;
		}
	},
	FINAL {
		@Override
		public char getAbbreviation() {
			return 'X';
		}

		@Override
		public boolean isFinal() {
			return true;
		}
	}, 
	ERROR {
		@Override
		public char getAbbreviation() {
			return 'E';
		}

		@Override
		public boolean isFinal() {
			return true;
		}
	},
	DELETING {
		@Override
		public char getAbbreviation() {
			return 'D';
		}
		
		@Override
		public boolean isFinal() {
			return false;
		}
	};

	public abstract char getAbbreviation();
	
	public abstract boolean isFinal();
	
	public static ProcessStatus fromAbbreviation(char abbreviation) {
		for (ProcessStatus s : values()) {
			if (s.getAbbreviation() == abbreviation) {
				return s;
			}
		}
		throw new IllegalArgumentException();
	}
}
