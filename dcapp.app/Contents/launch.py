#!/usr/bin/python

import os, platform, sys

ostype = os.getenv('OSTYPE')
if (ostype == None): ostype = platform.system().lower()

machtype = os.getenv('MACHTYPE')
if (machtype == None): machtype = platform.machine()

if (ostype == 'darwin'): osspec = 'MacOS'
else: osspec = ostype + '_' + machtype

myfullpath = os.path.abspath(__file__)
appcmd = os.path.join(os.path.dirname(myfullpath), osspec, os.path.basename(myfullpath))

myargs = [appcmd]
for arg in sys.argv[1:]: myargs.append(arg)

if os.path.isfile(appcmd): launch_app(myargs)
