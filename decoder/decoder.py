#!/usr/bin/python

import serial,os,re,datetime
from subprocess import call
import base91
import binascii
import urllib2
import io
import sys
import argparse
import getpass
import telnetlib
import time

# Parse arguments from terminal
parser = argparse.ArgumentParser(description='APRS/SSDV decoder')
parser.add_argument('-c', '--call', help='Callsign of the station', required=True)
parser.add_argument('-l', '--log', help='Name of the logfile')
parser.add_argument('-n', '--grouping', help='Amount packets that will be sent to the SSDV server in one request', default=1, type=int)
parser.add_argument('-d', '--device', help='Serial device (\'-\' for stdin)', default='-')
parser.add_argument('-b', '--baudrate', help='Baudrate for serial device', default=9600, type=int)
parser.add_argument('-s', '--server', help='SSDV server URL', default='https://ssdv.habhub.org/api/v0/packets')
args = parser.parse_args()

if args.device == 'I': # Connect to APRS-IS

	print('Connect to APRS-IS')
	try:
		tn = telnetlib.Telnet("rotate.aprs2.net", 14580, 3)
		tn.write("user %s filter u/APECAN\n" % args.call)
		print('Connected')
	except Exception as e:
		print('Could not connect to APRS-IS: %s' % str(e))
		print('exit...')
		sys.exit(1)

elif args.device is not '-': # Use serial connection (probably TNC)

	try:
		serr = serial.Serial(
			port=args.device,
			baudrate=args.baudrate,
		)
	except:
		sys.stderr.write('Error: Could not open serial port\n')
		sys.exit(1)


# Open logging file
if args.log is not None:
	try:
		f = open(args.log, 'a')
	except:
		sys.stderr.write('Error: Could not open logging file\n')
		sys.exit(1)

jsons = []

def decode_callsign(code):
	callsign = ''

	while code > 0:
		s = code % 40
		if   s == 0: callsign += '-'
		elif s < 11: callsign += chr(47 + s)
		elif s < 14: callsign += '-'
		else: callsign += chr(51 + s)
		code /= 40

	return callsign

def encode_callsign(callsign):
	x = 0
	for i in range(len(callsign)-1,-1,-1):
		x *= 40
		c = ord(callsign[i])
		if   c >= 65 and c <= 90:  x += c - 51
		elif c >= 97 and c <= 122: x += c - 83
		elif c >= 48 and c <= 57:  x += c - 47

	return x



def received_data(data):
	global jsons
	# Parse line and detect data
	m = re.search("(.*)\>APECAN(.*):\!(.*)", data)
	try:
		call = m.group(1).split(' ')[-1] # transmitter callsign
		data91 = m.group(3) # base91 encoded SSDV data (without SYNC, PacketType, Callsign, CRC, FEC)
		if len(m.group(2)) > 0:
			receiver = m.group(2).split(',')[-1]
		else:
			receiver = args.call
	except:
		return # message format incorrect (probably no APRS message or line cut off too short)

	if args.log is not None:
		f.write(data) # Log data to file

	data = base91.decode(data91) # Decode Base91

	if len(data) != 214:
		return # APRS message sampled too short

	# Encode callsign (ensure callsign has no more than 6 chars)
	bcall = call.split('-') # Split callsign and SSID
	if len(bcall) == 1: # No SSID available, so take the callsign
		bcall = bcall[0][0:6]
	elif(len(bcall[0]) < 5): # Callsign has 4 chars, so take it with the SSID
		bcall = bcall[0] + bcall[1][0:2]
	elif(len(bcall[0]) < 6): # Callsign has 5 chars, so take it with the last digit of the SSID
		bcall = bcall[0] + bcall[1][-1]
	else:
		bcall = bcall[0][0:6] # Callsign has 6 chars, so take the call without SSID

	data = binascii.unhexlify('66%08x' % encode_callsign(bcall)) + data

	# Calculate CRC for SSDV server
	crc = binascii.crc32(data) & 0xffffffff

	# Create message for SSDV server (and save to array)
	ssdv = '55' + binascii.hexlify(data) + ('%08x' % crc) + (64*'0')
	jsons.append("""{
		\"type\": \"packet\",
		\"packet\": \"""" + ssdv + """\",
		\"encoding\": \"hex\",
		\"received\": \"""" + datetime.datetime.now().isoformat('T')[:19] + """Z\",
		\"receiver\": \"""" + receiver + """\"
	}""")

	print datetime.datetime.now().isoformat('T') + ' Received packet call %s image %d packet %d' % (call, data[5], 0x100*data[6]+data[7])

	if len(jsons) >= args.grouping: # Enough packets collected, send them all to the server

		req = urllib2.Request(args.server)
		req.add_header('Content-Type', 'application/json')

		json = "{\"type\":\"packets\",\"packets\":[" + ",".join(jsons) + "]}" # Group all SSDV packets into a big JSON
		jsons = []

		try:
			error = True
			while error:
				print('Send to SSDV data server')
				try:
					result = urllib2.urlopen(req, "".join(json.split(' '))) # Send packets to server
					print('Response from Server: OK')
					error = False
				except urllib2.URLError, error:
					if error.code == 400:
						print('Response from Server: %s', error.read())
						error = False
					else:
						print 'SSDV-Server connection error... try again'

		except urllib2.HTTPError, error: # The server did not like our packets :(
			print 'Send to SSDV data server: failed (the server did not like our packets :( )'
			print error.read()

if args.device == 'I': # APRS-IS

	wdg = time.time() # Watchdog
	buf = ''
	while True:
		buf += tn.read_eager()
		if '\n' in buf:
			pbuf = buf.split('\n')
			for i in range(len(pbuf)-1):
				# Separate lines handled here

				# Watchdog reload
				if '# aprsc' in pbuf[i]:
					print('Ping from APRS-IS')
					wdg = time.time()
					continue

				# Data handling
				print(pbuf[i])
				received_data(pbuf[i])

			buf = pbuf[-1]

		# Watchdog reconnection
		if wdg + 30 < time.time():
			print('APRS-IS connection lost... reconnect')
			try:
				tn = telnetlib.Telnet("rotate.aprs2.net", 14580, 3)
				tn.write("user %s filter u/APECAN\n" % args.call)
				print('Connected')
				wdg = time.time()
			except Exception as e:
				print('Could not connect to APRS-IS: %s' % str(e))
				print('Try again...')

		time.sleep(0.1)

else: # stdin or serial

	while True:
		# Read data
		if args.device is '-':
			data = sys.stdin.readline()
		else:
			data = ''
			while True:
				b = serr.read(1)
				if b == '\r' or b == '\n':
					break
				data += b

		received_data(data)













