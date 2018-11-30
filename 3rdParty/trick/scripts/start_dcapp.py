##############################################################################
# PURPOSE:
#    (This provides a set of routines that allow dcapp instances to be
#     customized and launched from inside a Trick input file.)
#
# REFERENCE:
#    (Trick documentation.)
#
# ASSUMPTIONS AND LIMITATIONS:
#    ((Assumes that the trick and trick_ip variables are available globally.))
#
# PROGRAMMERS:
#    ((Michael McFarlane) (NASA) (August 2016) (--) ()))
##############################################################################

import os
import subprocess
import socket

# This specifies where start_dcapp will look for customization files
customization_dir = os.path.join(os.path.expanduser('~'), '.dcapp')

dcapp_user_args = ''
dcapp_local_client_count = 1
dcapp_local_args = [ '' ]
dcapp_remote_client_count = 0
dcapp_remote_args = [ ]
dcapp_remote_hosts = [ ]
dcapp_remote_displays = [ ]

def start_dcapp(dc_data_file, input_file_args = ''):

    global dcapp_user_args
    global dcapp_local_client_count
    global dcapp_local_args
    global dcapp_remote_client_count
    global dcapp_remote_args
    global dcapp_remote_hosts
    global dcapp_remote_displays

    # Don't start dcapp if we're just verifying the input file
    if (trick_ip.ip.verify_input != 0):
        print 'VERIFYING INPUT FILE: Not starting dcapp.'
        return

    # Define the path to the config file
    config_exec = os.path.join('externals', 'dcapp', 'dcapp.app', 'Contents', 'dcapp-config')

    # Define the path to the executable
    app_path = subprocess.check_output(config_exec + ' --exepath', shell=True)
    app_exec = os.path.join(app_path.strip(), 'dcapp')

    # Define the path to the dcapp specfile
    spec_file = os.path.join('externals', 'dcapp', 'displays', dc_data_file)

    # Get the port for the variable server from Trick
    var_server_port = str(trick.var_server_get_port())

    # Build the core command to execute
    app_cmnd = app_exec + ' ' + spec_file + ' TrickPort=' + var_server_port

    # Look for user arguments in the customization directories
    host_config_path = os.path.join(customization_dir + '_' + socket.gethostname(), 'config.py')
    user_config_path = os.path.join(customization_dir, 'config.py')
    if os.path.isfile(host_config_path):
        execfile(host_config_path)
    elif os.path.isfile(user_config_path):
        execfile(user_config_path)

    # Append user arguments and arguments specified in input file, if any
    app_cmnd += ' ' + dcapp_user_args +  ' ' + input_file_args

    # Execute launch command(s) customized to the appropriate environment(s)
    for i in range(dcapp_local_client_count):
        os.system(app_cmnd + ' ' + dcapp_local_args[i] + ' &')
    for i in range(dcapp_remote_client_count):
        os.system('ssh -x ' + dcapp_remote_hosts[i] + ' "setenv DISPLAY ' + dcapp_remote_displays[i] + '; cd ' + os.getcwd() + '; ' + app_cmnd + ' TrickHost=' + socket.gethostname() + ' ' + dcapp_remote_args[i] + ' &" &')


def customize_dcapp(user_args):

    global dcapp_user_args

    dcapp_user_args += user_args


def inhibit_local_dcapp_clients():

    global dcapp_local_client_count
    global dcapp_local_args

    dcapp_local_client_count = 0
    dcapp_local_args = [ ]


def add_local_dcapp_client(local_args = ''):

    global dcapp_local_client_count
    global dcapp_local_args

    dcapp_local_client_count += 1
    dcapp_local_args.append(local_args)


def inhibit_remote_dcapp_clients():

    global dcapp_remote_client_count
    global dcapp_remote_args
    global dcapp_remote_hosts
    global dcapp_remote_displays

    dcapp_remote_client_count = 0
    dcapp_remote_args = [ ]
    dcapp_remote_hosts = [ ]
    dcapp_remote_displays = [ ]


def add_remote_dcapp_client(remote_host, remote_display, remote_args = ''):

    global dcapp_remote_client_count
    global dcapp_remote_args
    global dcapp_remote_hosts
    global dcapp_remote_displays

    dcapp_remote_client_count += 1
    dcapp_remote_args.append(remote_args)
    dcapp_remote_hosts.append(remote_host)
    dcapp_remote_displays.append(remote_display)
