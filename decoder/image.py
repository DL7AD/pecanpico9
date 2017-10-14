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
def insert_image(sqlite, receiver, call, tim, posi, data, typ, server, grouping):
	global jsons

	if not (typ is 'I' and len(data) == 125) and not (typ is 'J' and len(data) == 124):
		return # APRS message has invalid type or length (or both)

	# Decode various meta data
	timd,x,y,z,teld,new = decode_position(tim, posi, None)
	imageID  = data[0]
	packetID = (data[1] << 8) | data[2]
	data = binascii.hexlify(data[3:]).decode("ascii")

	# Debug
	print('Received %s-Packet Image %d Packet %d' % (typ, imageID, packetID))

	# Insert
	sqlite.cursor().execute("""
		INSERT OR IGNORE INTO image (call,time,imageID,packetID,lat,lon,alt)
		VALUES (?,?,?,?,?,?,?)""",
		(call, int(timd.timestamp()), imageID, packetID, y, x, int(z))
	)
	sqlite.commit()

	if typ is 'I':

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

		data = ('67%08x%02x%04x' % (encode_callsign(bcall), imageID, packetID)) + data

		sqlite.cursor().execute("""
			UPDATE image
			SET data1 = ?
			WHERE call = ?
			AND time = ?
			AND imageID = ?
			AND packetID = ?""",
			(data, call, int(timd.timestamp()), imageID, packetID)
		)

	elif typ is 'J':

		sqlite.cursor().execute("""
			UPDATE image
			SET data2 = ?
			WHERE call = ?
			AND time = ?
			AND imageID = ?
			AND packetID = ?""",
			(data, call, int(timd.timestamp()), imageID, packetID)
		)

	sqlite.commit()

	# Get both data entries
	cur = sqlite.cursor()
	cur.execute("""
		SELECT data1,data2
		FROM image
		WHERE call = ?
		AND time = ?
		AND imageID = ?
		AND packetID = ?""",
		(call, int(timd.timestamp()), imageID, packetID)
	)
	row = cur.fetchall()[0]

	if row[0] != None and row[1] != None: # Both entries have been received

		data = ''.join(row)

		# Calculate CRC for SSDV server
		crc = binascii.crc32(binascii.unhexlify(data)) & 0xffffffff

		# Update CRC in DB
		sqlite.cursor().execute("""
			UPDATE image
			SET crc = ?
			WHERE call = ?
			AND time = ?
			AND imageID = ?
			AND packetID = ?""",
			("%08x" % crc, call, int(timd.timestamp()), imageID, packetID)
		)
		sqlite.commit()

		# SSDV decode
		cur = sqlite.cursor()
		cur.execute("SELECT GROUP_CONCAT('55' || data1 || data2 || crc, '') FROM image WHERE call = ? AND imageID = ? AND time = ? GROUP BY imageID ORDER BY packetID", (call, imageID, int(timd.timestamp())))
		name = 'html/images/%s-%d-%d.jpg' % (call.replace('-',''), int(timd.timestamp()), imageID)
		f = open(name, 'wb')
		process = Popen(['./ssdv', '-d'], stdin=PIPE, stdout=f, stderr=PIPE)
		process.stdin.write(binascii.unhexlify(cur.fetchall()[0][0]))
		dummy,err = process.communicate()
		f.close()

		# Create message for SSDV server (and save to array)
		ssdv = '55' + data + ('%08x' % crc)
		jsons.append("""{
			\"type\": \"packet\",
			\"packet\": \"""" + ssdv + """\",
			\"encoding\": \"hex\",
			\"received\": \"""" + datetime.datetime.now().isoformat('T')[:19] + """Z\",
			\"receiver\": \"""" + receiver + """\"
		}""")

		if len(jsons) >= grouping: # Enough packets collected, send them all to the server

			req = urllib.request.Request(server)
			req.add_header('Content-Type', 'application/json')

			json = "{\"type\":\"packets\",\"packets\":[" + ",".join(jsons) + "]}" # Group all SSDV packets into a big JSON
			jsons = []

			try:
				err = True
				while err:
					print('Send to SSDV data server')
					try:
						result = urllib.request.urlopen(req, "".join(json.split(' ')).encode("ascii")) # Send packets to server
						print('Response from SSDV-Server: OK')
						err = False
					except urllib.error.URLError as error:
						if not hasattr(error, 'code'): # (Bug in urllib) most likely network not available
							print('Error: Could not connect to SSDV-Server')
						elif error.code == 400:
							print('Response from SSDV-Server: %s' % error.read().decode('ascii').replace('\n',''))
							err = False
						else:
							print('SSDV-Server connection error... try again')

			except urllib.error.HTTPError as error: # The server did not like our packets :(
				print('Send to SSDV data server: failed (the server did not like our packets :( )')
				print(error.read())

