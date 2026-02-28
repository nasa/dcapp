##############################################################################
# Trick input file for stress test simulation (2000 variables)
##############################################################################

trick.var_server_set_port(7100)

trick.real_time_enable()
trick.itimer_enable()

trick.exec_set_software_frame(0.1)

trick.stop(600.0)

print("=" * 50)
print("Stress Test Simulation Started (2000 variables)")
print("Variable Server listening on port 7100")
print("Run dcapp with: ./dcapp ../stress.xml")
print("=" * 50)
