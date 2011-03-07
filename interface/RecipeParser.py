'''
statement         := MASHING | SPARGING | PAUSE | mash_volume | heat | hold_for
mash_volume       := MASH_VOLUME volume
heat              := HEAT temperature 
hold_for          := HOLD temperature FOR time
volume            := (INTEGER | FLOAT) volume_units
temperature       := (INTEGER | FLOAT) temperature_units
time              := (INTEGER | FLOAT) time_units
volume_units      := LITERS | MILLILITERS
temperature_units := CELSIUS | FAHRENHEIT
time_units        := MILLISECONDS | SECONDS | MINUTES | HOURS
'''

import struct

class RecipeParser(object):
    def __init__(self, tokens):
        self.__tokens = tokens
        self.__token_index = 0

    def parse(self):
        result = list()
        while self.__token_index < len(self.__tokens):

            next_statement = self.__statement()
            if next_statement:
                result.extend(next_statement)
            else:
                self.__fail()
        result.append('END')
        return result

    def __fail(self):
        token, recipe_str, line_number = self.__next_token()
        if token:
            raise SyntaxError, \
                  "Error on line %s of recipe: unexpected token '%s'." % \
                      (line_number, recipe_str)
        else:
            raise SyntaxError, 'Unexpected end of file.'

    def __next_token(self):
        if self.__token_index >= len(self.__tokens):
            return (None, None, None)

        token = self.__tokens[self.__token_index]
        self.__token_index += 1
        return token

    def __no_match(self):
        self.__token_index -= 1
        return None 

    def __statement(self):
        result = list()
        token, recipe_str, line_number = self.__next_token()

        if token == 'INITIALIZE':
            return ['INI']
        elif token == 'MASHING':
            return ['MSH', 'PAU']
        elif token == 'SPARGING':
            return ['SPG', 'PAU']
        elif token == 'PAUSE':
            return ['PAU']
        else:
            self.__token_index -= 1
            return self.__mash_volume() or self.__heat() or self.__hold_for()

    def __mash_volume(self):
        token = self.__next_token()[0]
        if token == 'MASH_VOLUME':
            volume = self.__volume() or self.__fail()
            return ['MWV', volume]
        else:
            return self.__no_match() 

    def __heat(self):
        token = self.__next_token()[0]
        if token == 'HEAT':
            temperature = self.__temperature() or self.__fail()
            return ['HEA', temperature]
        else:
            return self.__no_match() 

    def __hold_for(self):
        token = self.__next_token()[0]
        if token == 'HOLD':
            temperature = self.__temperature() or self.__fail()
            token = self.__next_token()[0]
            if token == 'FOR':
                time = self.__time() or self.__fail()
                return ['HLD', temperature, time]
            else:
                return self.__no_match() 
        else:
            return self.__no_match() 

    def __volume(self):
        num_type, number, line_number = self.__next_token()

        converter = self.__volume_units()
        if converter:
            if num_type == 'FLOAT':
                number = converter(float(number))
            elif num_type == 'INTEGER':
                number = converter(int(number))
            else:
                return self.__no_match()

            return struct.pack('L', number)
        else:
            return None

    def __volume_units(self):
        units = self.__next_token()[0]
        if units == 'LITERS':
            return lambda v: v * 1000 
        elif units == 'MILLILITERS':
            return lambda v: v
        else:
            return self.__no_match() 

    def __temperature(self):
        num_type, number, line_number = self.__next_token()

        converter = self.__temperature_units()
        if converter:
            if num_type == 'FLOAT':
                number = converter(float(number))
            elif num_type == 'INTEGER':
                number = converter(int(number))
            else:
                return self.__no_match()

            return struct.pack('f', number)
        else:
            return None

    def __temperature_units(self):
        units = self.__next_token()[0]
        if units == 'CELSIUS':
            return lambda v: v
        elif units == 'FAHRENHEIT':
            return lambda v: (v - 32) / 1.8
        else:
            return self.__no_match()
   
    def __time(self):
        num_type, number, line_number = self.__next_token()

        converter = self.__time_units()
        if converter:
            if num_type == 'FLOAT':
                number = converter(float(number))
            elif num_type == 'INTEGER':
                number = converter(int(number))
            else:
                return self.__no_match()

            return struct.pack('L', number)
        else:
            return None

    def __time_units(self):
        units = self.__next_token()[0]
        if units == 'MILLISECONDS':
            return lambda v: v
        elif units == 'SECONDS':
            return lambda v: v * 1000
        elif units == 'MINUTES':
            return lambda v: v * 1000 * 60
        elif units == 'HOURS':
            return lambda v: v * 1000 * 60 * 60
        else:
            return self.__no_match()

if __name__ == '__main__':
    import fileinput
    import RecipeLexer
    recipe = ''.join(list(fileinput.input()))
    print recipe
    tokens = RecipeLexer.RecipeLexer(recipe).tokens()
    print tokens
    print
    print RecipeParser(tokens).parse()

