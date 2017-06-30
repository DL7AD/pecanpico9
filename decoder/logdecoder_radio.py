#!/usr/bin/python

import base91

b91 = 'x5~Wh("vG"[[#9Bct5vW{oiZR|54uWs%!M3}tZ6y)=}#tDoSe+i9!+NWHH1[!eg@GLRDIo~%u;{(bCEU9H3|+v@AKy^Bm~`LSAv5*&EW{xI"[[)yg_WXCt{oiZR|AzXLs%_lAH@k6y(ggzbsOON/i9!+NWiGA'
dec = base91.decode(b91)

f = open('log.txt','wb')
f.write(dec)
f.close()

for i in range(16):
	tim = dec[8*i+0] + 256 * dec[8*i+1]
	lat = dec[8*i+2] + 256 * dec[8*i+3]
	lon = dec[8*i+4] + 256 * dec[8*i+5]
	alt = dec[8*i+6] + 256 * dec[8*i+7]

	month = tim / 4464
	day = (tim-month*4464) / 144
	hour = (tim-month*4464-day*144) / 6
	minute = tim-month*4464-day*144-hour*6

	lat = (float(lat*180)/65535)-90
	lon = (float(lon*360)/65535)-180

	print '%02d.%02d. %02d:%02d lat=%.3f, lon=%.3f alt=%d' % (day+1,month+1,hour,minute,lat,lon,alt)

