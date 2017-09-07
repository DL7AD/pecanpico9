#!/usr/bin/python

import serial,os,re,datetime
from subprocess import call
import base91
import binascii
import urllib2
import io
import sys
import argparse
import pygame, random
from pygame.locals import *
import pygame.time
from subprocess import *
from cStringIO import StringIO

send_to_server = False
SCREENX = 1024
SCREENY = 768
pygame.font.init()
myfont = pygame.font.SysFont('Comic Sans MS', 20)
textsurface = myfont.render('Callsign: DL7AD2 Image ID: 07 Resolution: 640x480', False, (0, 255, 255))
pygame.init()
screen = pygame.display.set_mode((SCREENX, SCREENY))
background = pygame.Surface(screen.get_rect().size)
displaygroup = pygame.sprite.RenderUpdates()
updategroup = pygame.sprite.Group()
clock = pygame.time.Clock()
pygame.display.set_caption('PecanRXGui  v.1.0.0 (Q)uit (s)end image')

# Parse arguments from terminal
parser = argparse.ArgumentParser(description='APRS/SSDV decoder')
parser.add_argument('-c', '--call', help='Callsign of the station')
parser.add_argument('-l', '--log', help='Name of the logfile')
parser.add_argument('-n', '--grouping', help='Amount packets that will be sent to the SSDV server in one request', default=1, type=int)
parser.add_argument('-d', '--device', help='Serial device (\'-\' for stdin)', default='-')
parser.add_argument('-b', '--baudrate', help='Baudrate for serial device', default=9600, type=int)
parser.add_argument('-s', '--server', help='Server URL', default='https://ssdv.habhub.org/api/v0/packets')
args = parser.parse_args()

if args.device == 'I': # Connect to APRS-IS
	aprsis = aprs.TCP('DL4MDW', aprs_filter='t/u u/APECAN')
	aprsis.start()

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
		logfile = open(args.log, 'a')
	except:
		sys.stderr.write('Error: Could not open logging file\n')
		sys.exit(1)

jsons = []
current_filename = 'data.csv'
buf = ''
jpg = ''
imgbuf = ''

def received_data(data):
	global jsons, old_filename, current_filename, send_to_server, buf, imgbuf

	if str(type(data)) == "<class 'aprs.classes.Frame'>": # APRS-IS

		call = str(data.source)
		aprs = data.text[3:]
		receiver = 'APRS-IS/' + str(data.path.pop())

	else: # serial or stdin

		# Parse line and detect data
		m = re.search("(.*)\>APECAN(.*):\{\{I(.*)", data)
		if m:
			try:
				call = m.group(1)
				aprs = m.group(3)
				receiver = 'bla'
			except:
				return # message format incorrect (probably no APRS message or line cut off too short)
		else:
			m = re.search("\[(.*)\]\[(.*)\] DATA \> (.*)", data)
			try:
				aprs = m.group(3)
			except:
				return

	if args.log is not None:
		logfile.write(data) # Log data to file

	data = base91.decode(aprs) # Decode Base91
	
	# Calculate CRC for SSDV server
	crc = binascii.crc32(data) & 0xffffffff
	# Create message for SSDV server (and save to array)
	ssdv = '55' + binascii.hexlify(data) + ('%08x' % crc) + (64*'0')

	if len(data) != 219: 
		return # APRS message sampled too short

	if (data[7] + data[6] * 256) == 0:
		buf = ''
	buf += binascii.unhexlify(ssdv)

	command = ['./ssdv', '-d']
	process = Popen(command, stdin=PIPE, stdout=PIPE, stderr=PIPE)
	process.stdin.write(buf)
	jpg,dummy = process.communicate()
	imgbuf = StringIO(jpg)

	if send_to_server is False:
		return
	else:
		if len(jsons) >= args.grouping:
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
						print 'Send to SSDV data server: failed (connection error :( trying again...)'

			except urllib2.HTTPError, error: # The server did not like our packets :(
				print 'Send to SSDV data server: failed (the server did not like our packets :( )'
				print error.read()

if args.device == 'I': # APRS-IS
	aprsis.receive(callback=received_data) # Register APRS callback
else:
	# read data from console ?
	while 1:
		for event in pygame.event.get():
			if event.type == QUIT:
				exit(0)

		if event.type == pygame.KEYDOWN:
			if event.key == pygame.K_q:
					exit(0)
			elif event.key == pygame.K_s:
					send_to_server^=True

		data = sys.stdin.readline() if args.device is '-' else ser.readline() # Read a line
		received_data(data)

		displaygroup.clear(screen, background)
		updategroup.update()
		filename = str("currximg.jpg")
		try:
			img=pygame.image.load(imgbuf)
			textsurface = myfont.render("Call: %s send: %d" % (args.call, send_to_server), False, (0, 255, 255))
			screen.blit(img,(0,0))
			screen.blit(textsurface,(0,0))
			pygame.display.flip()
			pygame.display.update(displaygroup.draw(screen))
		except Exception as e:
			print str(e)
			textsurface = myfont.render('Error %s' % (e), False, (255, 100, 100))
			screen.blit(textsurface,(0,0))
			pygame.display.flip()
			pygame.display.update(displaygroup.draw(screen))

		clock.tick(500)
