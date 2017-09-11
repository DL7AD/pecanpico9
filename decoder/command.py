#!/usr/bin/python

import serial,os,re,sys
import pygame
from pygame.locals import *
import pygame.time
from cStringIO import StringIO

try:
	ser = serial.Serial(port='/dev/ttyACM1')
except:
	sys.stderr.write('Error: Could not open serial port\n')
	sys.exit(1)


ser.write('\r\n')
ser.write('command '+str(int(sys.argv[1], 16))+' '+str(int(sys.argv[2], 16))+'\r\n')
ser.close()
