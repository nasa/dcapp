#!/usr/bin/python

import os, subprocess, sys

myfullpath = os.path.abspath(__file__)
mydir = os.path.dirname(myfullpath)
mybase = os.path.basename(myfullpath)

myscript = os.path.abspath(os.path.join(mydir, '..', '..', 'bin', 'dcapp-config'))

# The check_output method is much cleaner than Popen, but it requires Python 2.7 or higher
#osspec = subprocess.check_output([myscript, '--osspec']).strip()
osspec = subprocess.Popen([myscript, '--osspec'], stdout=subprocess.PIPE).communicate()[0].strip()

appcmd = os.path.join(mydir, osspec, mybase)

myargs = [appcmd]
for arg in sys.argv[1:]: myargs.append(arg)

if os.path.isfile(appcmd): launch_app(myargs)
else: print 'ERROR: Compiled ' + os.path.basename(appcmd) + ' binary doesn\'t exist.'
