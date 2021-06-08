import serial
import concurrent.futures
import queue
import re
import time


# One user supplied queue for sending events to user code
# One internal queue for receiving responses (if we don't have an active command
# then send it to user supplied queue)


class Event:
	COMMAND = 0
	RESPONSE = 1

	# TODO: add raw text and parsed data

	def __init__(self, type, data):
		self.type = type
		self.data = data


class SWT21:

	adc0_trig_pattern = re.compile(b'adc0 trig (\\d+) (\\d+) (\\d+) (\\d+)')
	adc0_clk_pattern = re.compile(b'ADC0 clk: (\\d+.\\d+)')
	adc_trig_pattern = re.compile(b'ADC trig (\\d+)\\+(\\d+)')

	def __init__(self, port, baudrate, event_queue):
		self.serial = serial.Serial(port, baudrate)
		self.queue = event_queue
		self.response_queue = queue.Queue()
		self.response_timeout = 0.01 # 10 ms
		self.current_command = None
		self.current_command_args = None

		self.executor = concurrent.futures.ThreadPoolExecutor(max_workers=1)
		self.executor.submit(self.read_serial)

		self.rx_flush()

		self.current_command = b'help'
		self.serial.write(b'help\n')
		response1 = self.response_queue.get()
		response2 = self.response_queue.get()
		if(response1 != b'help\n' or response2 != b'OK\n'):
			raise Exception('Invalid answer from device: {}'.format(response1 + response2))

		self.rx_flush()
		self.current_command = None


	def rx_flush(self):
		time.sleep(0.1)
		while not self.response_queue.empty():
			self.response_queue.get()


	def parse_serial_input(self, line):

		# Differentiate between answers and unsolicited commands

		if(line.startswith(b'CAN RX:')):
			self.queue.put(Event(Event.COMMAND, ('CAN RX', line)))

		elif(line.startswith(b'LIN RX:')):
			self.queue.put(Event(Event.COMMAND, ('LIN RX', line)))

		elif(line.startswith(b'UART:')):
			self.queue.put(Event(Event.COMMAND, ('UART', line)))

		elif(line.startswith(b'ADC0 clk:')):
			self.adc0_clk = self.parse_adc0_clk(line)
			print(self.adc0_clk)

		elif(line.startswith(b'ADC trig')):
			data = self.parse_adc_trig(line + self.serial.readline())
			if(data):
				self.queue.put(Event(Event.COMMAND, ('ADC trig', data)))

		else:
			if(self.current_command):
				self.response_queue.put(line)
			else:
				self.queue.put(Event(Event.RESPONSE, line))


	def read_serial(self):
		while True:
			try:
				line = self.serial.readline()
				self.parse_serial_input(line)
			except Exception as e:
				print(e)
				return


	def send_command(self, command):

		# Fetch adc0 trig here to be able to parse responses
		if(command.startswith(b'adc0 trig')):
			self.current_command_args = self.parse_adc0_trig(command)
			self.adc_data_range = [-1, -1]
			self.adc_data = []

		self.serial.write(command)


	def parse_adc0_clk(self, command):

		m = self.adc0_clk_pattern.match(command)

		if(not m):
			print('Bad ADC0 clk')
			return

		return float(m.group(1))


	def parse_adc0_trig(self, command):

		m = self.adc0_trig_pattern.match(command)

		if(not m):
			print('Bad adc0 trig')
			return

		return (int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4)))


	def parse_adc_trig(self, command):

		header, data, *tmp = command.split(b'\n')

		# Header format: ADC trig <start>+<len>
		m = self.adc_trig_pattern.match(header)
		if(not m):
			print('Bad ADC:', command)
			return

		start = int(m.group(1))
		end = int(m.group(2))

		values = [int(data[2*i:2*i+2], base=16) for i in range(len(data)//2)]

		self.adc_data += values

		if(self.adc_data_range[0] == -1):
			self.adc_data_range[0] = start

		self.adc_data_range[1] = start + end

		if(start + end >=
		   self.current_command_args[2] + self.current_command_args[3]):
			return (self.adc0_clk, self.current_command_args[2], self.adc_data_range[0], self.adc_data_range[1], self.adc_data)

		return None




