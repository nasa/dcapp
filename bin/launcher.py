#!/usr/bin/python

import sys, os

argstr = ''

for arg in sys.argv[1:]:
	argstr += ' ' + arg

mydir = os.path.dirname(os.path.abspath(__file__))
findlist = []

findlist.append(mydir + '/../dcapp.app/Contents/MacOS/dcapp')
findlist.append(mydir + '/../../../apps/dc/dcapp/dcapp.app/Contents/MacOS/dcapp')

for item in findlist:
    if os.path.isfile(item): os.system(item + argstr + ' &')
