#include <ctype.h>

#include "types.h"
#include "dict.h"

extern Dict* env;

Str* parseLine(Str line) {
	static Char temp[1024];
	Char* pos = temp;
	static Str args[256];
	Str* argpos = args;
	static Char varname[256];
	Char* varnamePos;
	
	Char quote = 0;
	void finishArg() {
		Str arg = strndup(temp, pos-temp);
		*argpos++ = arg;
		pos = temp;
	}
	Char c = *line;
	Char scan() {
		return (c = *++line);
	}
	while (c) {
		if (c == '$') {
			varnamePos = varname;
			while (isalnum(scan())) {
				*varnamePos++ = c;
			}
			*varnamePos = '\0';
			DictNode* item = Dict_get(env, varname);
			if (item) {
				Index len = strlen(item->value);
				memcpy(pos, item->value, len);
				pos += len;
			}
		} else if (quote) {
			if (c == quote)
				quote = 0;
			else
				*pos++ = c;
			scan();
		} else {
			switch (c) {
			when('"' orwhen '\''):
				quote = c;
			when(' '):
				finishArg();
			otherwise:
				*pos++ = c;
			}
			scan();
		}
	}
	finishArg();
	*argpos = NULL;
	return args;
}
