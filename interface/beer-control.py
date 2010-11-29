from SerialCommunicator import SerialCommunicator, DeviceSyncError
from Recipe import Recipe
import os, sys, time

def usage():
    print 'Usage: python %s <recipe_file> <output_log_file>' % sys.argv[0]
    sys.exit(1)

try:
    recipe_file_name = sys.argv[1]
    log_file_name = sys.argv[2]
except IndexError:
    usage()

s = SerialCommunicator()

try:
    s.open()
    pass
except Exception, e:
    print e
    sys.exit(1)

time.sleep(1)

log_file = open(log_file_name, 'a')

if s.read_current_status():
    print 'Program already running. Reset device to write new recipe.'
else:
    recipe = Recipe(open(recipe_file_name, 'r').read())
    print 'Sending recipe:'
    print "\n".join(recipe.human_readable())
    s.write_recipe(recipe)

while True:
    current_status = s.read_current_status()
    #current_status = 'SPARGING,HEA,0,0,31.00,24.25,40.00,40.00,1000,ON'
    if current_status == 'PAUSED':
        raw_input('Paused. Press Enter to continue.')
        s.serial.write('K')
    else:
        os.system("clear")
        log_file.write(time.asctime() + ',' + current_status + "\n")
        print SerialCommunicator.human_readable_status(current_status) 

