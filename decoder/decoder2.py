#!/usr/bin/python

import serial,os,re,sys
import pygame
from pygame.locals import *
import pygame.time
from cStringIO import StringIO

send_to_server = False
SCREENX = 640
SCREENY = 480
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


try:
	ser = serial.Serial(port='/dev/ttyACM1')
except:
	sys.stderr.write('Error: Could not open serial port\n')
	sys.exit(1)


ser.write('picture\r\n')

i=0
while True:
	line = ser.readline()
	m = re.search("\[(.*)\]\[(.*)\] DATA \> image\/jpeg\,(.*)", line)
	try:
		size = m.group(3)
	except:
		print line.strip()
		continue

	imgbuf = ser.read(int(size))
	f = open('data'+str(i)+'.jpg', 'wb')
	f.write(imgbuf)
	f.close()
	i+=1

	for event in pygame.event.get():
		if event.type == QUIT:
			exit(0)

	if event.type == pygame.KEYDOWN:
		if event.key == pygame.K_q:
				exit(0)
		elif event.key == pygame.K_s:
				send_to_server^=True

	displaygroup.clear(screen, background)
	updategroup.update()
	try:
		img=pygame.image.load(StringIO(imgbuf))
		textsurface = myfont.render("Call: %s send: %d" % ('USB', send_to_server), False, (0, 255, 255))
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

	ser.write('picture\r\n')











