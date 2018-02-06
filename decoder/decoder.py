#!/usr/bin/python3

import serial,re,io
import sys
import argparse
import telnetlib
import time
import sqlite3
import image
import position

# Parse arguments from terminal
parser = argparse.ArgumentParser(description='APRS/SSDV decoder')
parser.add_argument('-c', '--call', help='Callsign of the station', default='DL7AD')
parser.add_argument('-d', '--device', help='Serial device (\'-\' for stdin)', default='-')
parser.add_argument('-b', '--baudrate', help='Baudrate for serial device', default=9600, type=int)
parser.add_argument('-v', '--verbose', help='Activates more debug messages', action="store_true")
args = parser.parse_args()

# Open SQLite database
sqlite = sqlite3.connect("decoder.sqlite")
sqlite.cursor().execute("""
	CREATE TABLE IF NOT EXISTS position
	(
		call TEXT,
		rxtime INTEGER,
		org TEXT,

		reset INTEGER,
		id INTEGER,
		time INTEGER,

		adc_vsol INTEGER,
		adc_vbat INTEGER,
		pac_vsol INTEGER,
		pac_vbat INTEGER,
		pac_pbat INTEGER,
		pac_psol INTEGER,

		light_intensity INTEGER,

		gps_time INTEGER,
		gps_lock INTEGER,
		gps_sats INTEGER,
		gps_ttff INTEGER,
		gps_pdop INTEGER,
		gps_alt INTEGER,
		gps_lat INTEGER,
		gps_lon INTEGER,

		sen_i1_press INTEGER,
		sen_e1_press INTEGER,
		sen_e2_press INTEGER,
		sen_i1_temp INTEGER,
		sen_e1_temp INTEGER,
		sen_e2_temp INTEGER,
		sen_i1_hum INTEGER,
		sen_e1_hum INTEGER,
		sen_e2_hum INTEGER,

		stm32_temp INTEGER,
		si4464_temp INTEGER,

		sys_time INTEGER,
		sys_error INTEGER,
		PRIMARY KEY (call,reset,id,time)
	)
""")
sqlite.cursor().execute("""
	CREATE TABLE IF NOT EXISTS image
	(
		id INTEGER,
		call TEXT,
		rxtime INTEGER,
		imageID INTEGER,
		packetID INTEGER,
		data TEXT,
		PRIMARY KEY (call,id,packetID)
	)
""")


""" Packet handler for received APRS packets"""
def received_data(data):
	# Parse line and detect data
	# Position	(.*)\>APECAN(.*?):\/([0-9]{6}h)(.{13})(.*?)\|(.*)\|
	# Image		(.*)\>APECAN(.*?):\/([0-9]{6}h)(.{13})I(.*)
	# Log		(.*)\>APECAN(.*?):\/([0-9]{6}h)(.{13})L(.*)

	all = re.search("(.*)\>APECAN(.*?):", data)
	pos = re.search("(.*)\>APECAN(.*?):\!(.{13})(.*?)\|(.*)\|", data)
	dat = re.search("(.*)\>APECAN(.*?):\{\{(I|L)(.*)", data)

	if pos or dat:
		# Debug
		if args.verbose:
			print('='*100)
			print(data.strip())
			print('-'*100)

		call = all.group(1).split(' ')[-1]
		rxer = all.group(2).split(',')[-1]
		if not len(rxer): rxer = args.call

		if pos: # Position packet (with comment and telementry)

			comm = pos.group(4)
			position.insert_position(sqlite, call, comm, 'pos')

		elif dat: # Data packet

			typ  = dat.group(3)
			data = dat.group(4)

			if typ is 'I': # Image packet
				image.insert_image(sqlite, rxer, call, data)
			elif typ is 'L': # Log packet
				position.insert_position(sqlite, call, data, 'log')

if args.device == 'I': # Source APRS-IS

	print('Connect to APRS-IS')
	try:
		tn = telnetlib.Telnet("euro.aprs2.net", 14580, 3)
		tn.write(("user %s filter u/APECAN\n" % args.call).encode('ascii'))
		print('Connected')
	except Exception as e:
		print('Could not connect to APRS-IS: %s' % str(e))
		print('exit...')
		sys.exit(1)

	wdg = time.time() + 10 # Connection watchdog
	buf = ''
	while True:
		# Read data
		try:
			buf += tn.read_eager().decode('charmap')
		except EOFError: # Server has connection closed
			wdg = 0 # Tell watchdog to restart connection
		except UnicodeDecodeError:
			pass

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
				tn = telnetlib.Telnet("euro.aprs2.net", 14580, 3)
				tn.write(("user %s filter u/APECAN\n" % args.call).encode('ascii'))
				print('Connected')
				wdg = time.time() + 10
			except Exception as e:
				print('Could not connect to APRS-IS: %s' % str(e))
				print('Try again...')

		time.sleep(0.1)

elif args.device is '-': # Source stdin

	while True:
		received_data(sys.stdin.readline())

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

