import binascii
import urllib.request
import urllib.error
from datetime import datetime
from position import decode_position
from subprocess import *
import time
import threading
from shutil import copyfile

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

imageProcessor = None
imageData = {}
lock = threading.RLock()

def imgproc():
	global imageData

	while True:
		with lock:
			for _id in imageData:
				(call, data) = imageData[_id]

				filename = 'html/images/%s-%d.jpg' % (call.replace('-',''), _id)
				f = open(filename, 'wb')
				process = Popen(['./ssdv', '-d'], stdin=PIPE, stdout=f, stderr=PIPE)
				process.stdin.write(data)
				dummy,err = process.communicate()
				f.close()

				filename2 = 'html/images/%s.jpg' % (call.replace('-',''))
				copyfile(filename, filename2)

			imageData = {} # Clear data

		time.sleep(1)

w = time.time()
def insert_image(sqlite, receiver, call, data):
	global imageProcessor,imageData,w

	if len(data) != 174:
		return # APRS message has invalid type or length (or both)

	cur = sqlite.cursor()

	# Decode various meta data
	imageID  = data[0]
	packetID = (data[1] << 8) | data[2]
	data = binascii.hexlify(data[3:]).decode("ascii")

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

	data  = ('68%08x%02x%04x' % (encode_callsign(bcall), imageID, packetID)) + data
	data += "%08x" % (binascii.crc32(binascii.unhexlify(data)) & 0xffffffff)

	timd = int(datetime.now().timestamp())

	# Find image ID (or generate new one)
	_id = None
	cur.execute("SELECT id,packetID FROM image WHERE call = ? AND imageID = ? AND rxtime+15*60 >= ? ORDER BY rxtime DESC LIMIT 1", (call, imageID, timd))
	fetch = cur.fetchall()
	if len(fetch):
		_id = fetch[0][0]
		lastPacketId = fetch[0][1]

	if _id is None or lastPacketId > packetID:
		# Generate ID
		cur.execute("SELECT id+1 FROM image ORDER BY id DESC LIMIT 1")
		fetch = cur.fetchall()
		if len(fetch):
			_id = fetch[0][0]
		else: # No entries in the database
			_id = 0

	# Debug
	print('Received image packet %d Packet %d ServerID %d' % (imageID, packetID, _id))

	# Insert into database
	cur.execute("""
		INSERT OR IGNORE INTO image (call,rxtime,imageID,packetID,data,id)
		VALUES (?,?,?,?,?,?)""",
		(call, timd, imageID, packetID, data, _id)
	)

	if w+0.5 < time.time():
		sqlite.commit()
		w = time.time()

	with lock:
		cur.execute("SELECT GROUP_CONCAT('55' || data || '"+(144*'0')+"', '') FROM image WHERE id = ? GROUP BY id ORDER BY packetID", (_id,))
		data = cur.fetchall()[0][0]
		imageData[_id] = (call, binascii.unhexlify(data))

	if imageProcessor is None:
		imageProcessor = threading.Thread(target=imgproc)
		imageProcessor.start()

