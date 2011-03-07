import re

class RecipeLexer(object):
    TOKENS = [
        (r'(mash )?(water )?vol(\.|ume)?:?', 'MASH_VOLUME'),
        (r'initializ(e|ation):?', 'INITIALIZE'),
        (r'mash(ing)?:?', 'MASHING'),
        (r'sparg(e|ing):?', 'SPARGING'),
        (r'heat( to)?:?', 'HEAT'),
        (r'hold( at)?:?', 'HOLD'),
        (r'wait|pause', 'PAUSE'),
        (r'for:?', 'FOR'),
        (r'[-+]?[0-9]+\.[0-9]*', 'FLOAT'),
        (r'[-+]?[0-9]+', 'INTEGER'),
        (r'((deg\.?|degrees?) )?(c|celsius|centigrade)', 'CELSIUS'),
        (r'((deg\.?|degrees?) )?(f|fahrenheit)', 'FAHRENHEIT'),
        (r'ml|milliliters|millilitres', 'MILLILITERS'),
        (r'liters?|litres?|l', 'LITERS'),
        (r'ms|milliseconds?', 'MILLISECONDS'),
        (r'seconds?|secs?|s', 'SECONDS'),
        (r'minutes?|mins?|m', 'MINUTES'),
        (r'hours?|h', 'HOURS'),
    ]

    def __init__(self, recipe):
        self.__recipe = recipe
        self.__tokens = None

    def tokens(self):
        if not self.__tokens:
            self.__tokens = self.__lex()

        return self.__tokens

    def __lex(self):
        recipe_str = self.__recipe
        line_number = 1
        tokens = list()
        while recipe_str:
            match = re.match(r'\s+', recipe_str, flags=re.MULTILINE)
            if match:
                whitespace = match.group(0)
                line_number += len(re.findall(r'\n', whitespace))
                recipe_str = recipe_str[len(whitespace):]
                if not recipe_str:
                    break

            found_match = False
            for regex, token_name in RecipeLexer.TOKENS:
                match = re.match(regex, recipe_str, flags=re.IGNORECASE)
                if match:
                    matched_str = match.group(0)
                    token = token_name, matched_str, line_number
                    tokens.append(token)
                    recipe_str = recipe_str[len(matched_str):]
                    found_match = True
                    break

            if not found_match:
                next_nl_index = recipe_str.find("\n")
                if next_nl_index > 0:
                    current_line = recipe_str[:next_nl_index]
                else:
                    current_line = recipe_str
                raise SyntaxError, \
                      "Error on line %s of recipe: '%s'." % (line_number, current_line) 

        return tokens
        

if __name__ == '__main__':
    import fileinput
    recipe = ''.join(list(fileinput.input()))
    print recipe
    print RecipeLexer(recipe).tokens()
