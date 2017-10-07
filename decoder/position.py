from datetime import datetime,timedelta,timezone
import sqlite3
import base91

def insert_log(sqlite, call, data):

	if len(data)/10*10 != len(data): # Is not multiple of 8 bytes
		return # APRS message sampled too short

	for i in range(int(len(data)/10)):
		tim = (data[i*10+3] << 24) | (data[i*10+2] << 16) | (data[i*10+1] << 8) | data[i*10+0]
		lat = (data[i*10+5] << 8) | data[i*10+4]
		lon = (data[i*10+7] << 8) | data[i*10+6]
		alt = (data[i*10+9] << 8) | data[i*10+8]

		lat = float(lat * 180) / 65536 - 90
		lon = float(lon * 360) / 65536 - 180

		tim_stringified = datetime.utcfromtimestamp(tim).strftime("%Y-%m-%d %H:%M:%S")
		try:
			sqlite.cursor().execute("INSERT OR FAIL INTO position (call,time,org,lat,lon,alt) VALUES (?,?,'log',?,?,?)", (call, tim, lat, lon, alt))
			print("Decoded log from %s time %s => lat=%06.3f lon=%07.3f alt=%05d" % (call, tim_stringified, lat, lon, alt))
		except sqlite3.IntegrityError:
			print("Decoded log from %s time %s => lat=%06.3f lon=%07.3f alt=%05d already in db" % (call, tim_stringified, lat, lon, alt))

	sqlite.commit()

def decode_position(tim, posi, tel):
	# Decode latitude
	y3 = ord(posi[1]) - 33
	y2 = ord(posi[2]) - 33
	y1 = ord(posi[3]) - 33
	y0 = ord(posi[4]) - 33
	ye = y0 + y1*91 + y2*8281 + y3*753571
	y = -ye / 380926.0 + 90

	# Decode longitude
	x3 = ord(posi[5]) - 33
	x2 = ord(posi[6]) - 33
	x1 = ord(posi[7]) - 33
	x0 = ord(posi[8]) - 33
	xe = x0 + x1*91 + x2*8281 + x3*753571
	x = xe / 190463.0 - 180

	# Decode altitude
	z1 = ord(posi[10]) - 33
	z0 = ord(posi[11]) - 33
	ze = z0 + z1*91
	z = pow(1.002, ze) / 3.281

	# Decode time
	timd = datetime.now()
	timd = timd.replace(hour = int(tim[0:2]), minute = int(tim[2:4]), second = int(tim[4:6]), microsecond=0)
	if datetime.utcnow() < timd: # Packet was sampled yesterday
		timd -= timedelta(1)

	# Decode telemetry
	teld = [0]*6
	if tel != None:
		for i in range(6):
			t1 = ord(tel[i*2+0]) - 33
			t0 = ord(tel[i*2+1]) - 33
			teld.append(t0 + t1*91)

	return timd,x,y,z,teld

def insert_position(sqlite, call, tim, posi, comm, tel): #sqlite, call, data
	# Decode
	timd,x,y,z,teld = decode_position(tim, posi, tel)

	# Insert
	sqlite.cursor().execute("""
		INSERT OR REPLACE INTO position (call,time,org,lat,lon,alt,comment,sequ,tel1,tel2,tel3,tel4,tel5)
		VALUES (?,?,'pos',?,?,?,?,?,?,?,?,?,?)""",
		(call, int(timd.timestamp()), y, x, int(z), comm, teld[0], teld[1], teld[2], teld[3], teld[4], teld[5])
	)
	sqlite.commit()

	# Debug
	tim_stringified = timd.strftime("%Y-%m-%d %H:%M:%S")
	print("Decoded position from %s time %s => lat=%f lon=%f alt=%d comment=%s, sequ=%d tel=[%d,%d,%d,%d,%d]"
		% (call, tim_stringified, y, x, z, comm, teld[0], teld[1], teld[2], teld[3], teld[4], teld[5]))

