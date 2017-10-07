import binascii
import urllib.request
import urllib.error
import datetime
from position import decode_position
from subprocess import *

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

"""
Decodes an APRS/SSDV image packet (APRS header must be removed already)
sqlite - Database handle
reciever - The call of the receiving station
call - The call of the transmitter
data - Binary data
"""
def insert_image(sqlite, receiver, call, tim, posi, data, server, grouping):
	global jsons

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
	ssdv = '55' + binascii.hexlify(data).decode("ascii") + ('%08x' % crc) + (64*'0')
	jsons.append("""{
		\"type\": \"packet\",
		\"packet\": \"""" + ssdv + """\",
		\"encoding\": \"hex\",
		\"received\": \"""" + datetime.datetime.now().isoformat('T')[:19] + """Z\",
		\"receiver\": \"""" + receiver + """\"
	}""")

	# Decode various meta data
	timd,x,y,z,dummy = decode_position(tim, posi, None)
	imageID  = data[5]
	packetID = (data[6] << 8) | data[7]

	# Debug
	print('Received packet from %s image %d packet %d' % (call, imageID, packetID))

	# Insert
	sqlite.cursor().execute("""
		INSERT OR REPLACE INTO image (call,time,imageID,packetID,lat,lon,alt,data)
		VALUES (?,?,?,?,?,?,?,?)""",
		(call, int(timd.timestamp()), imageID, packetID, y, x, int(z), ssdv)
	)
	sqlite.commit()

	# SSDV decode
	cur = sqlite.cursor()
	cur.execute("SELECT GROUP_CONCAT(data,'') FROM image WHERE call = ? AND imageID = ? AND time = ? GROUP BY imageID ORDER BY packetID", (call, imageID, int(timd.timestamp())))
	name = 'html/images/%s-%d-%d.jpg' % (call.replace('-',''), int(timd.timestamp()), imageID)
	f = open(name, 'wb')
	process = Popen(['./ssdv', '-d'], stdin=PIPE, stdout=f, stderr=PIPE)
	process.stdin.write(binascii.unhexlify(cur.fetchall()[0][0]))
	dummy,err = process.communicate()
	f.close()

	if len(jsons) >= grouping: # Enough packets collected, send them all to the server

		req = urllib.request.Request(server)
		req.add_header('Content-Type', 'application/json')

		json = "{\"type\":\"packets\",\"packets\":[" + ",".join(jsons) + "]}" # Group all SSDV packets into a big JSON
		jsons = []

		try:
			error = True
			while error:
				print('Send to SSDV data server')
				try:
					result = urllib.request.urlopen(req, "".join(json.split(' ')).encode("ascii")) # Send packets to server
					print('Response from Server: OK')
					error = False
				except urllib.error.URLError as error:
					if error.code == 400:
						print('Response from Server: %s', error.read())
						error = False
					else:
						print('SSDV-Server connection error... try again')

		except urllib.error.HTTPError as error: # The server did not like our packets :(
			print('Send to SSDV data server: failed (the server did not like our packets :( )')
			print(error.read())

