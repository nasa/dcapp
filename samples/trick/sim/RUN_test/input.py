##############################################################################
# Trick input file for cannon simulation with variable server
##############################################################################

# Enable the variable server on port 7000
trick.var_server_set_port(7000)

# Set the simulation to run in real-time mode
trick.real_time_enable()
trick.itimer_enable()

# Set the software frame to 0.1 seconds (10 Hz)
trick.exec_set_software_frame(0.1)

# Run for 130 seconds (physics runs at 1/24 speed, so ~5.4s of physics time)
trick.stop(130.0)

# Print startup message
print("=" * 50)
print("Cannon Simulation Started")
print("Variable Server listening on port 7000")
print("Run dcapp with: ./dcapp ../trick.xml")
print("=" * 50)
