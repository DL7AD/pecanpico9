from datetime import datetime,timedelta,timezone
import sqlite3
import base91
import struct

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
			sqlite.cursor().execute("INSERT OR FAIL INTO position (call,time,org,lat,lon,alt,isnew) VALUES (?,?,'log',?,?,?,1)", (call, tim, lat, lon, alt))
			print("Decoded log from %s time %s => lat=%06.3f lon=%07.3f alt=%05d" % (call, tim_stringified, lat, lon, alt))
		except sqlite3.IntegrityError:
			print("Decoded log from %s time %s => lat=%06.3f lon=%07.3f alt=%05d already in db" % (call, tim_stringified, lat, lon, alt))

	sqlite.commit()

def decode_position(posi, tel):
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

	# Time
	timd = datetime.now(timezone.utc)

	# Decode GPS Fix Type
	isnew = ((ord(posi[12])-33) >> 5) & 0x1

	# Decode telemetry
	teld = [0]*6
	if tel != None:
		for i in range(6):
			t1 = ord(tel[i*2+0]) - 33
			t0 = ord(tel[i*2+1]) - 33
			teld.append(t0 + t1*91)

	return timd,x,y,z,teld,isnew

def insert_position(sqlite, call, posi, comm, tel): #sqlite, call, data
	# Decode position
	timd,x,y,z,teld,isnew = decode_position(posi, tel)

	# Decode comment
	data = base91.decode(comm)
	(adc_vsol,adc_vbat,pac_vsol,pac_vbat,pac_pbat,pac_psol,light_intensity,
	 gps_lock,gps_sats,gps_ttff,gps_pdop,gps_alt,gps_lat,
	 gps_lon,sen_i1_press,sen_e1_press,sen_e2_press,sen_i1_temp,sen_e1_temp,
	 sen_e2_temp,sen_i1_hum,sen_e1_hum,sen_e2_hum,dummy2,stm32_temp,
	 si4464_temp,reset,_id,gps_time,sys_time,sys_error) = struct.unpack('HHHHhhHBBBBHiiIIIhhhBBBBhhHIIII', data[:72])

	# Insert
	sqlite.cursor().execute(
		"""INSERT INTO position (call,rxtime,org,adc_vsol,adc_vbat,pac_vsol,pac_vbat,pac_pbat,pac_psol,light_intensity,gps_lock,gps_sats,gps_ttff,gps_pdop,gps_alt,gps_lat,gps_lon,sen_i1_press,sen_e1_press,sen_e2_press,sen_i1_temp,sen_e1_temp,sen_e2_temp,sen_i1_hum,sen_e1_hum,sen_e2_hum,sys_error,stm32_temp,si4464_temp,reset,id,sys_time,gps_time)
		VALUES (?,?,'pos',?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)""",
		(call,int(timd.timestamp()), adc_vsol,adc_vbat,pac_vsol,pac_vbat,pac_pbat,pac_psol,light_intensity,gps_lock,gps_sats,gps_ttff,gps_pdop,gps_alt,gps_lat,gps_lon,sen_i1_press,sen_e1_press,sen_e2_press,sen_i1_temp,sen_e1_temp,sen_e2_temp,sen_i1_hum,sen_e1_hum,sen_e2_hum,sys_error,stm32_temp,si4464_temp,reset,_id,sys_time,gps_time)
	)
	sqlite.commit()

	# Debug
	tim_stringified = timd.strftime("%Y-%m-%d %H:%M:%S")
	print("Decoded position from %s time %s => lat=%f lon=%f alt=%d new=%d comment=%s, sequ=%d tel=[%d,%d,%d,%d,%d]"
		% (call, tim_stringified, y, x, int(z), isnew, comm, teld[0], teld[1], teld[2], teld[3], teld[4], teld[5]))






















