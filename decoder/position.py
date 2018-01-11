from datetime import datetime,timedelta,timezone
import sqlite3
import base91
import struct

def insert_position(sqlite, call, comm, typ):
	# Decode comment
	data = base91.decode(comm)
	(adc_vsol,adc_vbat,pac_vsol,pac_vbat,pac_pbat,pac_psol,light_intensity,
	 gps_lock,gps_sats,gps_ttff,gps_pdop,gps_alt,gps_lat,
	 gps_lon,sen_i1_press,sen_e1_press,sen_e2_press,sen_i1_temp,sen_e1_temp,
	 sen_e2_temp,sen_i1_hum,sen_e1_hum,sen_e2_hum,dummy2,stm32_temp,
	 si4464_temp,reset,_id,gps_time,sys_time,sys_error) = struct.unpack('HHHHhhHBBBBHiiIIIhhhBBBBhhHIIII', data[:72])

	# Insert
	rxtime = int(datetime.now(timezone.utc).timestamp())
	sqlite.cursor().execute(
		"""INSERT INTO position (call,rxtime,org,adc_vsol,adc_vbat,pac_vsol,pac_vbat,pac_pbat,pac_psol,light_intensity,gps_lock,gps_sats,gps_ttff,gps_pdop,gps_alt,gps_lat,gps_lon,sen_i1_press,sen_e1_press,sen_e2_press,sen_i1_temp,sen_e1_temp,sen_e2_temp,sen_i1_hum,sen_e1_hum,sen_e2_hum,sys_error,stm32_temp,si4464_temp,reset,id,sys_time,gps_time)
		VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)""",
		(call,rxtime,typ,adc_vsol,adc_vbat,pac_vsol,pac_vbat,pac_pbat,pac_psol,light_intensity,gps_lock,gps_sats,gps_ttff,gps_pdop,gps_alt,gps_lat,gps_lon,sen_i1_press,sen_e1_press,sen_e2_press,sen_i1_temp,sen_e1_temp,sen_e2_temp,sen_i1_hum,sen_e1_hum,sen_e2_hum,sys_error,stm32_temp,si4464_temp,reset,_id,sys_time,gps_time)
	)
	sqlite.commit()

	# Debug
	print('Received %s packet packet Call=%s Reset=%d ID=%d' % (typ, call, reset, _id))

