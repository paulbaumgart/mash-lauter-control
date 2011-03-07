import os
import serial
import time

# SPARGING,HEA,0,0,31.00,24.25,40.00,40.00,1000,ON
OUTPUT_TEMPLATE = \
"""
%s:
    Stage type:         %s
    Time in stage:      %s
    Duration of stage:  %s
    HLT temperature:    %s
    Grain temperature:  %s
    Setpoint:           %s
    Target temperature: %s
    Heater duty cycle:  %s
    Pump state:         %s
"""

class DeviceSyncError(Exception):
    def __init__(self, expected, output, command=None):
        self.value = 'Error: Expected: "' + expected \
                     + '", but got: "' + output \
                     + '". Device out of sync.'
        if command:
            self.value = 'Failed sending command: "' + command + '". ' + self.value

    def __str__(self):
        return self.value

class SerialCommunicator(object):
    def open(self):
        try:
            device = filter(lambda s: s[:6] == 'ttyUSB', os.listdir('/dev/'))[0]
        except:
            raise Exception("Can't open USB serial connection. Is the microcontroller connected?")

        self.serial = serial.Serial('/dev/' + device, 9600, timeout=5)
        self.serial.flushInput()
        self.serial.flushOutput()

    def write_recipe(self, recipe):
        commands = recipe.serial_commands()

        for command in commands:
            self.serial.write(command)
            self.ensure_next_output_line_is('OK', command) 

    def read_current_status(self):
       return self.serial.readline().strip()

    def ensure_next_output_line_is(self, expected, command=None):
        output = self.serial.readline().strip()
        if output != expected:
            raise DeviceSyncError(expected, output, command)

    @staticmethod
    def human_readable_status(status):
        def ms_to_minsecs(ms):
            mins = int(ms) / 1000 / 60
            secs = int(ms)/1000 - mins * 60
            return "%d:%02d" % (mins, secs)

        try:
            status_items = status.split(',')
            if int(status_items[3]) == 0:
                status_items[2:4] = ['N/A', 'N/A']
            else:
                status_items[2:4] = map(ms_to_minsecs, status_items[2:4])
            output = OUTPUT_TEMPLATE % tuple(status_items) 
            return output
        except:
            return status

