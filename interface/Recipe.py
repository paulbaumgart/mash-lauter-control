import struct

class RecipeSyntaxError(Exception):
    def __init__(self, line, message):
        self.value = "\"" + line.strip() + "\": " + message

    def __str__(self):
        return self.value
        
class Recipe(object):
    MASHING, SPARGING = range(2)
    def __init__(self, mode):
        if mode != Recipe.MASHING and mode != Recipe.SPARGING:
            raise ValueError('mode must be either Recipe.MASHING or Recipe.SPARGING')
        self.mode = mode
        self.temp_duration_pairs = list()

    def add_line(self, line):
        parts = map(lambda x: x.strip().strip(':').lower(), line.split(','))
        try:
            temp, temp_scale = parts[0].split()
        except ValueError:
            raise RecipeSyntaxError(parts[0], "Expected: 2 values separated by a space: <temperature> <scale>. Got: " + str(len(parts[0].split())) + " value(s).")
        try:
            time, time_units = parts[1].split()
        except ValueError:
            raise RecipeSyntaxError(parts[1], "Expected: 2 values separated by a space: <time> <units>. Got: " + str(len(parts[1].split())) + " value(s).")
        try:
            temp = int(temp)
        except ValueError:
            raise RecipeSyntaxError(temp, "Can't parse temperature value.")

        if temp_scale[0].lower() == 'f':
            temp = (temp - 32) * 5 / 9
        elif temp_scale[0].lower() != 'c':
            raise RecipeSyntaxError(temp_scale, "Can't parse temperature scale (expected: 'C' or 'F').")

        try:
            time = int(time)
        except ValueError:
            raise RecipeSyntaxError(time, "Can't parse time value.")

        if time_units[:2].lower() == 'ms' or time_units[:3].lower() == 'mil': # milliseconds
            pass
        elif time_units[:1].lower() == 's': # seconds
            time = time * 1000
        elif time_units[:1].lower() == 'm': # minutes
            time = time * 1000 * 60
        elif time_units[:1].lower() == 'h': # hours
            time = time * 1000 * 60 * 60
        else:
            raise RecipeSyntaxError(time_units, "Can't parse time units (expected: 'h', 'm', 's', or 'ms').")

        self.temp_duration_pairs.append((temp, time))

    def human_readable(self):
        strings = ['MASHING' if self.mode == Recipe.MASHING else 'SPARGING']
        strings.extend([u"%5.2f \u00B0C for %5.2f minutes" % (pair[0], pair[1] / 1000 / 60) for pair in self.temp_duration_pairs])
        return "\n".join(strings)

    def serial_commands(self):
        commands = [
            'BEG',
            'MSH' if self.mode == Recipe.MASHING else 'SPG',
            struct.pack('L', len(self.temp_duration_pairs))
        ]

        packed_temp_duration_pairs = [(struct.pack('f', t[0]), struct.pack('L', t[1])) \
                                     for t in self.temp_duration_pairs]

        commands.extend([x for y in packed_temp_duration_pairs for x in y])
        commands.append('END')

        check_value = [(struct.unpack('f', t[0])[0], struct.unpack('L', t[1])[0]) \
                          for t in zip(commands[3:-1:2], commands[4:-1:2])]
        assert check_value == self.temp_duration_pairs, \
                   "%s should equal %s" % (check_value, self.temp_duration_pairs)

        return commands

    @staticmethod
    def recipes_from_iterable(iterable):
        recipes = list()
        current_recipe = None
        for line_num, line in enumerate(iterable):
            try:
                parts = map(lambda x: x.strip().strip(':').lower(), line.split(','))
                if len(parts) == 1: # start a new recipe
                    if parts[0]:
                        if current_recipe:
                            recipes.append(current_recipe)
                            current_recipe = None

                        if parts[0] == 'mashing':
                            current_recipe = Recipe(Recipe.MASHING)
                        elif parts[0] == 'sparging':
                            current_recipe = Recipe(Recipe.SPARGING)
                        else:
                            raise RecipeSyntaxError(line, "Can't determine whether you mean MASHING or SPARGING.")
                elif current_recipe:
                    current_recipe.add_line(line)
                else:
                    raise SyntaxError("Error in line: \"" + line.strip() \
                                          + "\". Specify MASHING or SPARGING first.")
            except RecipeSyntaxError as e:
                e.value = 'Error in recipe on line %s: ' % line_num + e.value
                raise e

        if current_recipe:
            recipes.append(current_recipe)

        return recipes


if __name__ == '__main__':
    import fileinput, sys
    try:
        rs = Recipe.recipes_from_iterable(fileinput.input())
    except RecipeSyntaxError as e:
        print str(e)
        sys.exit(1)
    for r in rs:
        print r.serial_commands() 
        print r.human_readable()
        
