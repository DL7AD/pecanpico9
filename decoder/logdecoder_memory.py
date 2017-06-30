

#typedef struct {
#	uint16_t year;
#	uint8_t month;
#	uint8_t day;
#	uint8_t hour;
#	uint8_t minute;
#	uint8_t second;
#	uint16_t millisecond;
#} ptime_t;

#typedef struct {
#	uint32_t id;			// Serial ID
#	ptime_t time;			// GPS time
#
#	// GPS
#	bool gps_lock;			// True = on last try GPS has been locked
#	int32_t gps_lat;		// Latitude in *10^7
#	int32_t gps_lon;		// Longitude in *10^7
#	int32_t gps_alt;		// Altitude in meter
#	uint8_t gps_sats;		// Satellites used for solution
#	uint8_t gps_ttff;		// Time to first fix in seconds
#
#	// Voltage and current measurement
#	uint16_t adc_vsol;		// Current solar voltage in mV
#	uint16_t adc_vbat;		// Current battery voltage in mV
#	uint16_t adc_vusb;		// Current USB voltage in mV
#	int16_t adc_pbat;		// Average battery current (since last track point)
#	int16_t adc_isol;		// Current solar shot current
#
#	// BME280 (on board)
#	uint32_t air_press;		// Airpressure in Pa*10 (in 0.1Pa)
#	uint16_t air_hum;		// Rel. humidity in %*10 (in 0.1%)
#	int16_t air_temp;		// Temperature in degC*100 (in 0.01C)
#} trackPoint_t;

class TrackPoint:
	def __init__(self, d):
		self.id          = ord(d[0]) + ord(d[1])*0x100 + ord(d[2])*0x10000 + ord(d[3])*0x1000000

		self.year        = ord(d[4]) + ord(d[5])*0x100
		self.month       = ord(d[6])
		self.day         = ord(d[7])
		self.hour        = ord(d[8])
		self.minute      = ord(d[9])
		self.second      = ord(d[10])
		self.millisecond = ord(d[11]) + ord(d[12])*0x100
		# 13 always 0
		self.gps_lock    = ord(d[14])
		# 15 always 0
		self.gps_lat     = (ord(d[16]) + ord(d[17])*0x100 + ord(d[18])*0x10000 + ord(d[19])*0x1000000) / 10000000.0
		self.gps_lon     = (ord(d[20]) + ord(d[21])*0x100 + ord(d[22])*0x10000 + ord(d[23])*0x1000000) / 10000000.0
		self.gps_alt     = ord(d[24]) + ord(d[25])*0x100 + ord(d[26])*0x10000 + ord(d[27])*0x1000000
		self.gps_sats    = ord(d[28])
		self.gps_ttff    = ord(d[29])

		self.vsol        = ord(d[30]) + ord(d[31])*0x100
		self.vbat        = ord(d[32]) + ord(d[33])*0x100
		self.vusb        = ord(d[34]) + ord(d[35])*0x100
		self.pbat        = ord(d[36]) + ord(d[37])*0x100
		self.isol        = ord(d[38]) + ord(d[39])*0x100

		self.air_press   = ord(d[40]) + ord(d[41])*0x100 + ord(d[42])*0x10000 + ord(d[43])*0x1000000
		self.air_hum     = ord(d[44]) + ord(d[45])*0x100
		self.air_temp    = ord(d[46]) + ord(d[47])*0x100

		if self.gps_lat > 0x7FFFFFFF: self.gps_lat -= 0x100000000
		if self.gps_lon > 0x7FFFFFFF: self.gps_lon -= 0x100000000
		if self.pbat > 0x7FFF: self.pbat -= 0x10000
		if self.air_temp > 0x7FFF: self.air_temp -= 0x10000
		self.air_temp /= 100.0

		self.hex = '0x'
		for i in range(len(d)):
			self.hex += ('0' if ord(d[i])<16 else '') + hex(ord(d[i]))[2:]



with open('log.bin', 'rb') as f:
	while True:
		d = f.read(48)
		
		if len(d) != 48:
			break

		tp = TrackPoint(d)

		if tp.id != 0xFFFFFFFF:

			print(
				"%d,%d-%02d-%02d,%02d:%02d:%02d:%04d,%d,%.5f,%.5f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.2f" %
				(tp.id,tp.year,tp.month,tp.day,tp.hour,tp.minute,tp.second,tp.millisecond,tp.gps_lock,tp.gps_lat,tp.gps_lon,
				tp.gps_alt,tp.gps_sats,tp.gps_ttff,tp.vbat,tp.vsol,tp.vusb,tp.pbat,tp.isol,tp.air_press,tp.air_hum,tp.air_temp)
			)


































