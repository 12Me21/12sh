#include <ctype.h>
#include <stdio.h>

#include "types.h"
#include "dict.h"
#include "parse.h"

extern Dict* env;

void freeArgs(Str* args) {
	for (; *args; args++)
		free(*args);
}

Str* parseLine(Str line, Bool multi) {
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
		if (quote != '\'' && c == '$') {
			varnamePos = varname;
			while (isalnum(scan())) {
				*varnamePos++ = c;
			}
			if (varnamePos == varname) {
				*pos++ = '$';
			} else {
				*varnamePos = '\0';
				Str* item = Dict_get(env, varname);
				if (item) {
					Index len = strlen(*item);
					memcpy(pos, *item, len);
					pos += len;
				}
			}
		} else if (quote != '\'' && c == '\\') {
			scan();
			switch (c) {
			when('\0'): goto end;
			when('1'): *pos++ = '\1';
			when('2'): *pos++ = '\2';
			when('n'): *pos++ = '\n';
			when('e'): *pos++ = '\033';
			when('t'): *pos++ = '\t';
			when('b'): *pos++ = '\b';
			when('f'): *pos++ = '\f';
			when('r'): *pos++ = '\r';
			when('a'): *pos++ = '\a';
			otherwise:
				*pos++ = c;
			}
			scan();
		} else if (quote) {
			if (c == quote)
				quote = 0;
			else
				*pos++ = c;
			scan();
		} else switch (c) {
			when('"' orwhen '\''):
				quote = c;
			when(' '):
				if (multi) {
					finishArg();
					do {
						scan();
					} while (c == ' ');
				} else {
					*pos++ = c;
					scan();
				}
			otherwise:
				*pos++ = c;
				scan();
			}
	}
 end:
	finishArg();
	*argpos = NULL;
	return args;
}
