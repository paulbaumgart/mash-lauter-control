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


if __name__ == '__main__':
    import fileinput
    from Recipe import Recipe

    s = SerialCommunicator()
    s.open()
    time.sleep(1)
    recipe_str = ''.join(list(fileinput.input()))
    recipe = Recipe(recipe_str)
    print "\n".join(recipe.human_readable())
    s.write_recipe(recipe)

    while True:
        current_status = s.read_current_status()
        if current_status == 'PAUSED':
            raw_input('Paused. Press Enter to continue.')
            s.serial.write('K')
        else:
            print current_status
