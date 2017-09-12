#!/usr/bin/python

import os, subprocess, sys

myfullpath = os.path.abspath(__file__)
mydir = os.path.dirname(myfullpath)
mybase = os.path.basename(myfullpath)

myscript = os.path.abspath(os.path.join(mydir, '..', '..', 'bin', 'dcapp-config'))
osspec = subprocess.check_output([myscript, "--osspec"]).strip()

appcmd = os.path.join(mydir, osspec, mybase)

myargs = [appcmd]
for arg in sys.argv[1:]: myargs.append(arg)

if os.path.isfile(appcmd): launch_app(myargs)
else: print 'ERROR: Compiled ' + os.path.basename(appcmd) + ' binary doesn\'t exist.'
