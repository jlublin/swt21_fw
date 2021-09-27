#!/usr/bin/env python3

import threading
import queue
import time
import sys
from construct import *

import swt21
import fdx

GROUP_RECV = 1001
GROUP_SEND = 1002

class FdxEvent:

	def __init__(self, packet, commands):
		self.packet = packet
		self.commands = commands

def rx_thread(queue, fdx_conn):
	while(True):
		packet, commands = fdx_conn.recv()
		queue.put(FdxEvent(packet, commands))

CanFrame = Struct(
	"data_size" / Int32ul,
	"id" / Int32ul,
	"dlc" / Int8ul,
	"data" / Byte[8]
	)

def create_can_frame(id, dlc, data):
	return Container(data_size=13, id=id, dlc=dlc, data=data+[0]*(8-len(data)))

class CanFrameClass:
	def __init__(self, id=0, dlc=0, data=[]):
		self.id = id
		self.dlc = dlc
		self.data = data

	def build(self):
		data_str = ''.join(['{:02x}'.format(x) for x in self.data])
		return '{:x}#{}'.format(self.id, data_str)

	def parse(self, string):
		frame_str = string[8:]
		id_str, data_str = frame_str.split('#')

		self.id = int(id_str, 16)
		self.dlc = len(data_str)//2
		self.data = [int(data_str[2*i:2*(i+1)],16) for i in range(self.dlc)]


if(__name__ == '__main__'):

	if(len(sys.argv) < 4 or len(sys.argv) > 7):
		print('''\
Usage: {} <COM-port> <brp> <tseg1> <tseg2> [<sjw>] [<FDX addr>] [<FDX port>]

brp - CAN baud rate prescaler (2-128, even)
tseg1 - CAN time segment 1 in time quanta (1-16)
tseg2 - CAN time segment 2 in time quanta  (1-8)
sjw - CAN synchronization jump width (1-4), default: 1
FDX addr - CANoe FDX server address, default: 127.0.0.1
FDX port - CANoe FDX server port, default 2809

CAN bitrate can be calculated from
bitrate = 80 000 / (brp * (1 + tseg1 + tseg2)) kbit / s

Example configuration:
brp = 10
tseg1 = 8
tseg2 = 7
=> bitrate = 500 kbit/s
''')
		sys.exit(1)

	# Read command arguments
	serial_port = sys.argv[1]
	can_brp = int(sys.argv[2])
	can_tseg1 = int(sys.argv[3])
	can_tseg2 = int(sys.argv[4])

	can_sjw = 1
	if(len(sys.argv) > 5):
		can_sjw = int(sys.argv[5])

	addr = '127.0.0.1'
	if(len(sys.argv) > 6):
		addr = sys.argv[6]

	port = 2809
	if(len(sys.argv) > 7):
		port = int(sys.argv[7])

	# Setup event queue
	queue = queue.Queue()

	# Setup serial connection to labkit
	labkit = swt21.SWT21(serial_port, 2000000, queue)

	labkit.send_command('can config brp {}\n'.format(can_brp).encode())
	queue.get()
	queue.get()

	labkit.send_command('can config tseg_1 {}\n'.format(can_tseg1).encode())
	queue.get()
	queue.get()

	labkit.send_command('can config tseg_2 {}\n'.format(can_tseg2).encode())
	queue.get()
	queue.get()

	labkit.send_command('can config sjw {}\n'.format(can_sjw).encode())
	queue.get()
	queue.get()

	labkit.send_command('can rx on\n'.encode())
	queue.get()
	queue.get()

	# Setup FDX connection to CANoe
	fdx_conn = fdx.Fdx(addr, port)

	rx_thread = threading.Thread(target=rx_thread, args=(queue, fdx_conn), daemon=True)
	rx_thread.start()

	fdx_conn.send([fdx.create_free_running_request(GROUP_RECV, 8, 0, 0)])

	print('FDX ready!')

	while(True):

		event = queue.get()

		if(type(event) == swt21.Event):
			if(event.type == swt21.Event.COMMAND):
				command = event.data
				if(command[0] == 'CAN RX'):
					frame = CanFrameClass()
					frame.parse(command[1].decode())
					send_frame = create_can_frame(frame.id, frame.dlc, frame.data)
					fdx_conn.send([fdx.create_data_exchange(GROUP_SEND, CanFrame.build(send_frame))])

		elif(type(event) == FdxEvent):
			for cmd in event.commands:
				if(cmd.code == fdx.Command.DataExchange):
					if(cmd.group_id == GROUP_RECV):
						frame = CanFrame.parse(bytes(cmd.data))
						frame2 = CanFrameClass(frame.id, frame.dlc, frame.data[:frame.dlc])
						labkit.send_command('can send {}\n'.format(frame2.build()).encode())
