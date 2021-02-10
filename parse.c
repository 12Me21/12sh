#include <ctype.h>
#include <stdio.h>

#include "types.h"
#include "dict.h"
#include "parse.h"

extern Dict* env;

void* memdup(void* src, size_t len) {
	void* n = malloc(len) CRITICAL;
	memcpy(n, src, len);
	return n;
}

void Source_free(Source* source) {
	if (!source)
		return;
	if (source->type == Source_FILE)
		free(source->filepath);
	else if (source->type == Source_STRING)
		free(source->string);
	Source_free(source->next);
	free(source);
}

void CommandLine_free(CommandLine* cmd) {
	if (!cmd)
		return;
	Str* args = cmd->argv;
	for (; *args; args++)
		free(*args);
	free(cmd->argv);
	Source_free(cmd->redirections);
	CommandLine_free(cmd->next);
	free(cmd);
}

CommandLine* parseLine(Str line) {
	static Char temp[1024];
	Char* pos = temp;
	static Str args[256];
	Str* argpos = args;
	static Char varname[256];
	Char* varnamePos;
	
	Char quote = 0;

	Char c = *line;
	Char scan() {
		return (c = *++line);
	}

	void finishArg(Bool quoted) {
		if (quoted || pos > temp) {
			Str arg = strndup(temp, pos-temp);
			*argpos++ = arg;
		}
		if (!quoted)
			while (c == ' ')
				scan();
		pos = temp;
	}
	CommandLine* ALLOC(cmd);

	void finishPart(Bool next) {
		*argpos = NULL;
		int items = argpos - args + 1;
		Str* args2 = memdup(args, sizeof(Str*)*(items+1));
		cmd->argv = args2;
		cmd->command = args2[0];
		argpos = args;
		if (next) {
			cmd->pipeout = true;
			
			CommandLine* ALLOC(cmd2);
			cmd->next = cmd2;
			cmd = cmd2;
			cmd->next = NULL;
			cmd->bg = false;
			cmd->pipeout = false;
			cmd->redirections = NULL;
			cmd->argv = NULL;
		} else {
			cmd->next = NULL;
		}
	}
	cmd->bg = false;
	cmd->pipeout = false;
	cmd->redirections = NULL;
	CommandLine* firstCommand = cmd;
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
				scan();
			when(' '):
				scan();
				finishArg(false);
			when('&'):
				scan();
				finishArg(false);
				cmd->bg = true;
			when('|'):
				scan();
				finishArg(false);
				finishPart(true);
			when('>'):
				scan();
				finishArg(false);
			otherwise:
				*pos++ = c;
				scan();
			}
	}
 end:
	finishArg(false);
	finishPart(false);
	return firstCommand;
}
