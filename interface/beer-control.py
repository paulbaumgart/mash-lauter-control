from SerialCommunicator import SerialCommunicator, DeviceSyncError
from Recipe import Recipe
import os, ossaudiodev, sys, time, wave

def usage():
    print 'Usage: python %s <recipe_file> <output_log_file>' % sys.argv[0]
    sys.exit(1)

def play_sound(filename):
    sound = wave.open(filename,'rb')

    try:
        dsp = ossaudiodev.open('/dev/dsp','w')
    except IOError:
        return

    dsp.setparameters(ossaudiodev.AFMT_S16_NE,
                      sound.getnchannels(),
                      sound.getframerate())
    sound_data = sound.readframes(sound.getnframes())
    sound.close()
    dsp.write(sound_data)
    dsp.close()


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

current_status = s.read_current_status()

if current_status:
    if current_status[:5] == 'ERROR':
        print current_status
    else:
        print 'ERROR: Program already running. Reset the device and try again.'
    sys.exit(1)
else:
    recipe = Recipe(open(recipe_file_name, 'r').read())
    print 'Sending recipe:'
    print "\n".join(recipe.human_readable())
    try:
        s.write_recipe(recipe)
    except DeviceSyncError as err:
        print err
        sys.exit(1)

had_error = False
while True:
    current_status = s.read_current_status()
    #current_status = 'SPARGING,HEA,0,0,31.00,24.25,40.00,40.00,1000,ON'
    if current_status == 'PAUSED':
        if had_error:
            play_sound('interface/alarm.wav')
        else:
            play_sound('interface/ding.wav')

        raw_input('Paused. Press Enter to continue.')
        s.serial.write('K')
        had_error = False
    else:
        os.system("clear")
        log_file.write(time.asctime() + ',' + current_status + "\n")
        output = SerialCommunicator.human_readable_status(current_status) 
        if output[:5] == 'ERROR':
            had_error = True
        print output


