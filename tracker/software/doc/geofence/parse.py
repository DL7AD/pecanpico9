# This is an updating script which parses the current geofencing from the software
# into a KML file (geofencing.kml) which can be displayed by geofencing.html.

import re
import webbrowser

_open = re.compile('^(.*?)coord_t (.*?)\[\] = \{(.*?)$')
_close = re.compile('^\};$')
_value = re.compile('^(.*?)\{(.*?),(.*?)\}(.*?)$')

region = {}
colorId = 0
colors = {
	'america':		'0xff 0xff 0x33',
	'china':		'0xff 0x00 0x00',
	'southkorea':	'0x00 0xff 0x00',
	'japan':		'0x00 0x00 0xff',
	'southeastAsia':'0xff 0xff 0x33',
	'australia':	'0x00 0xff 0x00',
	'newzealand':	'0x00 0x00 0xff',
	'argentina':	'0x00 0x00 0xff',
	'brazil':		'0xff 0x00 0x00'
}
freq = {}
remInts = lambda x: ''.join([i for i in x if not i.isdigit()]) # Remove integers from string

# Parse
print('Parsing')
with open("../../math/geofence.c", "r") as f:
	for line in f:
		m = _open.match(line)
		if m: # Found region start
			region[m.group(2)] = []
			currRegion = m.group(2)

			# Parse frequency
			with open("../../modules/radio.h", "r") as fb:
				_freq = re.compile('^(.*?)APRS_FREQ_' + currRegion.upper() + '(.*?)$')
				for lineb in fb:
					m = _freq.match(lineb)
					if m:
						freq[currRegion] = float(m.group(2))/1000000

			print('> %s, %.3f MHz' % (currRegion.title(), freq[remInts(currRegion)]))
			continue
		
		if _close.match(line): # Found region end
			currRegion = None
			continue

		m = _value.match(line)
		if m and currRegion is not None: # Found region data
			region[currRegion].append((float(m.group(3)), float(m.group(2))))

# Write to KML
print('Write to KML')
with open("geofencing.kml", "w") as f:
	f.write('<?xml version="1.0" encoding="UTF-8"?>\n')
	f.write('<kml xmlns="http://earth.google.com/kml/2.2">\n')
	f.write('<Document>\n')

	for name in region:
		f.write('<Placemark>\n')
		f.write('<name>%s %.3fMHz %s</name>\n' % (remInts(name).title(), freq[remInts(name)], colors[remInts(name)]))
		f.write('<MultiGeometry>\n')
		f.write('<Polygon>\n')
		f.write('<extrude>1</extrude>\n')
		f.write('<altitudeMode>relativeToGround</altitudeMode>\n')
		f.write('<outerBoundaryIs>\n')
		f.write('<LinearRing>\n')
		f.write('<coordinates>\n')
		for point in region[name]:
			f.write(str(point[0]/10000000.0) + ',' + str(point[1]/10000000.0) + ',0\n')
		f.write('</coordinates>\n')
		f.write('</LinearRing>\n')
		f.write('</outerBoundaryIs>\n')
		f.write('</Polygon>\n')
		f.write('</MultiGeometry>\n')
		f.write('</Placemark>\n')

	f.write('</Document>\n')
	f.write('</kml>\n')

print('Open KML in Browser')
webbrowser.open_new('geofencing.html')
