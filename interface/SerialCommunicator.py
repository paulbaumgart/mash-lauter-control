import os
import serial
import time

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

        self.serial = serial.Serial('/dev/' + device, 9600, timeout=10)
        self.serial.flushInput()
        self.serial.flushOutput()
        self.ensure_next_output_line_is('PROGRAM RUNNING')

    def write_recipe(self, recipe):
        commands = recipe.serial_commands()

        for command in commands:
            self.serial.write(command)
            self.ensure_next_output_line_is('OK', command) 

        self.ensure_next_output_line_is('RUNNING SCRIPT')

    def read_current_status(self):
       return self.serial.readline().strip() 

    def ensure_next_output_line_is(self, expected, command=None):
        output = self.serial.readline().strip()
        if output != expected:
            raise DeviceSyncError(expected, output, command)


if __name__ == '__main__':
    import fileinput, sys
    from Recipe import *
    s = SerialCommunicator()
    try:
        s.open()
        rs = Recipe.recipes_from_iterable(fileinput.input())
        s.write_recipe(rs[0])
    except DeviceSyncError as e:
        print e.value
        sys.exit(1)

    while True:
        print s.read_current_status()
