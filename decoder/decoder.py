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

	tn = telnetlib.Telnet("rotate.aprs2.net", 14580)
	tn.write("user DL7AD filter u/APECAN\n")

elif args.device is not '-': # Use serial connection (probably TNC)

	try:
		serr = serial.Serial(
			port=args.device,
			baudrate=args.baudrate,
		)
	except:
		sys.stderr.write('Error: Could not open serial port\n')
		sys.exit(1)

	ser = io.TextIOWrapper(io.BufferedRWPair(serr, serr, 1), newline = '\r', line_buffering = True) # Define Newline as \r


# Open logging file
if args.log is not None:
	try:
		f = open(args.log, 'a')
	except:
		sys.stderr.write('Error: Could not open logging file\n')
		sys.exit(1)

jsons = []

def received_data(data):
	global jsons

	# Parse line and detect data
	m = re.search("(.*)\>APECAN(.*):\{\{I(.*)", data)
	try:
		call = m.group(1)
		aprs = m.group(3)
		receiver = args.call
	except:
		return # message format incorrect (probably no APRS message or line cut off too short)

	if args.log is not None:
		f.write(data) # Log data to file

	data = base91.decode(aprs) # Decode Base91

	if len(data) != 219:
		return # APRS message sampled too short

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

	print datetime.datetime.now().isoformat('T') + ' Received packet call %02x%02x%02x%02x image %d packet %d' % (data[1], data[2], data[3], data[4], data[5], data[7] + data[6] * 256)

	if len(jsons) >= args.grouping: # Enough packets collected, send them all to the server

		req = urllib2.Request(args.server)
		req.add_header('Content-Type', 'application/json')

		json = "{\"type\":\"packets\",\"packets\":[" + ",".join(jsons) + "]}" # Group all SSDV packets into a big JSON
		jsons = []

		try:
			error = True
			while error:
				try:
					result = urllib2.urlopen(req, "".join(json.split(' '))) # Send packets to server
					print 'Send to SSDV data server: OK'
					error = False
				except urllib2.URLError, error:
					if error.code == 400:
						print 'The SSDV server indicated a faulty packet: ' + error.read()
						error = False
					else:
						print 'Send to SSDV data server: failed (connection error :( trying again...)'

		except urllib2.HTTPError, error: # The server did not like our packets :(
			print 'Send to SSDV data server: failed (the server did not like our packets :( )'
			print error.read()

if args.device == 'I': # APRS-IS

	buf = ''
	while True:
		buf += tn.read_eager()
		if '\n' in buf:
			pbuf = buf.split('\n')
			for i in range(len(pbuf)-1):
				print(pbuf[i])
				received_data(pbuf[i])
			buf = pbuf[-1]
		time.sleep(0.1)

else: # stdin or serial

	while True:
		data = sys.stdin.readline() if args.device is '-' else ser.readline() # Read a line
		received_data(data)

