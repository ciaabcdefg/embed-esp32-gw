run_linux:
	pio run -t upload --upload-port /dev/ttyUSB0 -t monitor --monitor-port /dev/ttyUSB0

monitor_linux:	
	pio device monitor --port /dev/ttyUSB0

run:
	pio run -t upload --upload-port COM6 -t monitor --monitor-port COM6

monitor:
	pio device monitor --port COM6