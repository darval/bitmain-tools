#
#
#

set_voltage: set_voltage_new.c
	arm-linux-gnueabihf-gcc -Wall set_voltage_new.c -o set_voltage

clean:
	rm set_voltage
