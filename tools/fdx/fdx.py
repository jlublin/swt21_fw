#!/usr/bin/env python3

import socket
from construct import *

FDX_SIGNATURE = b'CANoeFDX'
FDX_MAJOR = 2
FDX_MINOR = 0

class Command:
	Start = 1
	Stop = 2
	Key = 3
	Status = 4
	DataExchange = 5
	DataRequest = 6
	DataError = 7
	FreeRunningRequest = 8
	FreeRunningCancel = 9
	StatusRequest = 10
	SeqNumError = 11

### Commands

FdxCommand = Struct(
	"command_size" / Int16ul,
	"code" / Int16ul
	)

FdxDataRequest = Struct(
	"command_size" / Int16ul,
	"code" / Int16ul,
	"group_id" / Int16ul
	)

def create_data_request(group_id):
	return Container(
		command_size = 6,
		code = Command.DataRequest,
		group_id = group_id
		)

FdxDataExchange = Struct(
	"command_size" / Int16ul,
	"code" / Int16ul,
	"group_id" / Int16ul,
	"data_size" / Int16ul,
	"data" / Byte[this.data_size]
	)

def create_data_exchange(group_id, data):
	return Container(
		command_size = 8 + len(data),
		code = Command.DataExchange,
		group_id = group_id,
		data_size = len(data),
		data = data
		)

FdxFreeRunningRequest = Struct(
	"command_size" / Int16ul,
	"code" / Int16ul,
	"group_id" / Int16ul,
	"flags" / Int16ul,
	"cycle_time" / Int32ul,
	"first_duration" / Int32ul
	)

def create_free_running_request(group_id, flags, cycle_time, first_duration):
	return Container(
		command_size = 16,
		code = Command.FreeRunningRequest,
		group_id = group_id,
		flags = flags,
		cycle_time = cycle_time,
		first_duration = first_duration
		)

FdxStatus = Struct(
	"command_size" / Int16ul,
	"code" / Int16ul,
	"state" / Enum(Int8ul, not_running=1, pre_start=2, running=3, stop=4),
	"_reserved" / Byte[3],
	"timestamp" / Int64ul
	)

FdxStatusRequest = Struct(
	"command_size" / Int16ul,
	"code" / Int16ul
	)

def create_status_request():
	return Container(
		command_size = 4,
		code = Command.StatusRequest
		)

### Packet
FdxHeader = Struct(
	"signature" / Byte[8],
	"major" / Int8ul,
	"minor" / Int8ul,
	"num_commands" / Int16ul,
	"seq" / Int16ul,
	"flags" / Int8ul,
	"_reserved" / Int8ul
	)

FdxPacket = Struct(
	"header" / FdxHeader,
	"commands" / GreedyBytes
	)

def create_packet(seq, commands):
	header = Container(
		signature = FDX_SIGNATURE,
		major = FDX_MAJOR,
		minor = FDX_MINOR,
		num_commands = len(commands),
		seq = seq,
		flags = 0,
		_reserved = 0
		)

	bytes = bytearray()

	for cmd in commands:
		if(cmd.code == Command.StatusRequest):
			bytes += FdxStatusRequest.build(cmd)

		elif(cmd.code == Command.DataRequest):
			bytes += FdxDataRequest.build(cmd)

		elif(cmd.code == Command.DataExchange):
			bytes += FdxDataExchange.build(cmd)

		elif(cmd.code == Command.FreeRunningRequest):
			bytes += FdxFreeRunningRequest.build(cmd)

		else:
			raise Exception('Unknown command {}'.format(cmd.code))

	return Container(
		header = header,
		commands = bytes)


def parse_packet(data):

	packet = FdxPacket.parse(data)
	commands = []

	i = 0
	while(i < len(packet.commands)):
		cmd = FdxCommand.parse(packet.commands[i:])

		if(cmd.command_size < 4):
			return

		if(cmd.code == Command.Status):
			commands.append(FdxStatus.parse(packet.commands[i:]))

		elif(cmd.code == Command.DataExchange):
			commands.append(FdxDataExchange.parse(packet.commands[i:]))

		else:
			commands.append(cmd)

		i += cmd.command_size

	return (packet, commands)


def rx_thread(queue, fdx):
	while(True):

		packet, commands = fdx.recv()
		print(packet.header.seq, commands)

		queue.put(('RX', packet, commands))


class Fdx:

	SIGNATURE = b'CANoeFDX'
	MAJOR = 2
	MINOR = 1

	def __init__(self, addr, port):
		self.seq = 0
		self.host_addr = addr
		self.host_port = port

		self.recv_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

		self.recv_socket.bind(('', 0))
		addr, self.port = self.recv_socket.getsockname()

		self.send_socket = socket.socket(fileno=self.recv_socket.fileno())

	def send(self, commands):
		packet = create_packet(self.seq, commands)
		self.send_socket.sendto(FdxPacket.build(packet), (self.host_addr, self.host_port))

		if(self.seq == 0x7fff):
			self.seq = 1
		else:
			self.seq += 1

	def recv(self):
		data, addr = self.recv_socket.recvfrom(4096)
		packet, commands = parse_packet(data)

		return packet, commands
