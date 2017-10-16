#!/usr/bin/python3

import serial,re
import base91
import sys
import argparse
import telnetlib
import time
import sqlite3
import image
import position

# Parse arguments from terminal
parser = argparse.ArgumentParser(description='APRS/SSDV decoder')
parser.add_argument('-c', '--call', help='Callsign of the station', default='N0CALL')
parser.add_argument('-n', '--grouping', help='Amount packets that will be sent to the SSDV server in one request', default=1, type=int)
parser.add_argument('-d', '--device', help='Serial device (\'-\' for stdin)', default='-')
parser.add_argument('-b', '--baudrate', help='Baudrate for serial device', default=9600, type=int)
parser.add_argument('-s', '--server', help='SSDV server URL', default='https://ssdv.habhub.org/api/v0/packets')
args = parser.parse_args()

# Open SQLite database
sqlite = sqlite3.connect("decoder.sqlite")
sqlite.cursor().execute("""
	CREATE TABLE IF NOT EXISTS position
	(
		call TEXT,
		time INTEGER,
		org TEXT,
		lat FLOAT,
		lon FLOAT,
		alt INTEGER,
		isnew INTEGER,
		comment TEXT,
		sequ INTEGER,
		tel1 INTEGER,
		tel2 INTEGER,
		tel3 INTEGER,
		tel4 INTEGER,
		tel5 INTEGER,
		PRIMARY KEY (call, time)
	)
""")
sqlite.cursor().execute("""
	CREATE TABLE IF NOT EXISTS image
	(
		call TEXT,
		time INTEGER,
		imageID INTEGER,
		packetID INTEGER,
		lat FLOAT,
		lon FLOAT,
		alt INTEGER,
		data1 TEXT,
		data2 TEXT,
		crc TEXT,
		PRIMARY KEY (call, time, imageID, packetID)
	)
""")


""" Packet handler for received APRS packets"""
def received_data(data):
	# Parse line and detect data
	# Position	(.*)\>APECAN(.*?):\/([0-9]{6}h)(.{13})(.*?)\|(.*)\|
	# Image		(.*)\>APECAN(.*?):\/([0-9]{6}h)(.{13})I(.*)
	# Log		(.*)\>APECAN(.*?):\/([0-9]{6}h)(.{13})L(.*)

	all = re.search("(.*)\>APECAN(.*?):\/([0-9]{6}h)(.{13})", data)
	pos = re.search("(.*)\>APECAN(.*?):\/([0-9]{6}h)(.{13})(.*?)\|(.*)\|", data)
	dat = re.search("(.*)\>APECAN(.*?):\/([0-9]{6}h)(.{13})(I|J|L)(.*)", data)

	if all:
		# Debug
		print('='*100)
		print(data)
		print('-'*100)

		call = all.group(1).split(' ')[-1]
		rxer = all.group(2).split(',')[-1]
		if not len(rxer): rxer = args.call
		tim  = all.group(3)
		posi = all.group(4)

		if pos: # Position packet (with comment and telementry)

			comm = pos.group(5)
			tel  = pos.group(6)
			position.insert_position(sqlite, call, tim, posi, comm, tel)

		elif dat: # Data packet

			typ  = dat.group(5)
			data_b91 = dat.group(6)
			data = base91.decode(data_b91) # Decode Base91

			if typ is 'I' or typ is 'J': # Image packet
				image.insert_image(sqlite, rxer, call, tim, posi, data, typ, args.server, args.grouping)
			elif typ is 'L': # Log packet
				position.insert_log(sqlite, call, data)

if args.device == 'I': # Source APRS-IS

	print('Connect to APRS-IS')
	try:
		tn = telnetlib.Telnet("rotate.aprs2.net", 14580, 3)
		tn.write(("user %s filter u/APECAN\n" % args.call).encode('ascii'))
		print('Connected')
	except Exception as e:
		print('Could not connect to APRS-IS: %s' % str(e))
		print('exit...')
		sys.exit(1)

	wdg = time.time() + 1 # Connection watchdog
	buf = ''
	while True:
		# Read data
		try:
			buf += tn.read_eager().decode('ascii')
		except EOFError: # Server has connection closed
			wdg = 0 # Tell watchdog to restart connection

		# Line handler
		if '\n' in buf:
			pbuf = buf.split('\n')
			for i in range(len(pbuf)-1):
				# Separate lines handled here

				# Watchdog reload
				if '# aprsc' in pbuf[i]:
					print('Ping from APRS-IS')
					wdg = time.time() + 30
					continue

				# Data handling
				received_data(pbuf[i])

			buf = pbuf[-1]

		# Watchdog reconnection
		if wdg < time.time():
			print('APRS-IS connection lost... reconnect')
			try:
				tn = telnetlib.Telnet("rotate.aprs2.net", 14580, 3)
				tn.write(("user %s filter u/APECAN\n" % args.call).encode('ascii'))
				print('Connected')
				wdg = time.time() + 1
			except Exception as e:
				print('Could not connect to APRS-IS: %s' % str(e))
				print('Try again...')

		time.sleep(0.1)

elif args.device is '-': # Source stdin

	while True:
		data = sys.stdin.readline()
		received_data(data)

else: # Source Serial connection

	try:
		serr = serial.Serial(
			port=args.device,
			baudrate=args.baudrate,
		)
	except:
		sys.stderr.write('Error: Could not open serial port\n')
		sys.exit(1)

	while True:
		data = ''
		while True:
			b = serr.read(1)
			if chr(b[0]) == '\r' or chr(b[0]) == '\n':
				break
			data += chr(b[0])

		received_data(data)

