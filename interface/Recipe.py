import struct
from RecipeLexer import RecipeLexer
from RecipeParser import RecipeParser

class Recipe(object):
    def __init__(self, recipe_str):
        tokens = RecipeLexer(recipe_str).tokens()
        self.__commands = RecipeParser(tokens).parse()

    def human_readable(self):
        index = 0
        output = list()
        while index < len(self.__commands):
            command = self.__commands[index]
            if command == 'INI':
                output.append('INITIALIZE:')
            elif command == 'MSH':
                output.append('MASHING:')
            elif command == 'SPG':
                output.append('SPARGING:')
            elif command == 'PAU':
                output.append("\tPAUSE")
            elif command == 'MWV':
                index += 1
                volume = self.__commands[index]
                output.append("\tMASH WATER VOLUME: %s mL" % struct.unpack('L', volume))
            elif command == 'HEA':
                index += 1
                temperature = self.__commands[index]
                output.append("\tHEAT TO: %s C" % struct.unpack('f', temperature))
            elif command == 'HLD':
                index += 1
                temperature = self.__commands[index]
                index += 1
                time = self.__commands[index]
                output.append("\tHOLD AT: %s C FOR: %s seconds" %
                              (struct.unpack('f', temperature)[0],
                               float(struct.unpack('L', time)[0]) / 1000))
            elif command == 'END':
                pass
            else:
                raise ValueError, command

            index += 1

        return output

    def serial_commands(self):
        return self.__commands

if __name__ == '__main__':
    import fileinput
    recipe_str = ''.join(list(fileinput.input()))
    recipe = Recipe(recipe_str)
    print recipe.serial_commands() 
    print
    print "\n".join(recipe.human_readable())
        
