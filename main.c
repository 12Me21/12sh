#include <readline/readline.h>
#include <readline/history.h>
#include <curses.h>

#include "types.h"
#include "dict.h"

typedef char* Str;

/*char prompt[] = {
	't', 'e', 's', 't', RL_PROMPT_START_IGNORE,
	'\x1B', '[', '3', '1', 'm', RL_PROMPT_END_IGNORE,
	'$', RL_PROMPT_START_IGNORE,
	'\x1B', '[', 'm', RL_PROMPT_END_IGNORE,
	' ', '\0',
	};//*/

char prompt[1024];

Dict* env;

Dict* makeEnvDict(Str* envp) {
	Dict* new = Dict_new(100);
	for (; *envp; envp++) {
		Str name, value;
		Str sep = strchr(*envp, '=');
		if (sep) {
			name = strndup(*envp, sep-*envp);
			value = strdup(sep+1);
		} else {
			name = strdup(*envp);
			value = "";
		}
		Dict_add(new, name)->value = value;
	}
	return new;
}

Str updatePrompt(Dict* env) {
	Str pwd = Dict_get(env, "PWD")->value;
}

int main(int argc, Str* argv, Str* envp) {
	env = makeEnvDict(envp);
	while (1) {
		
		Str line = readline(prompt);
		if (!line)
			break;
		DictNode* item = Dict_get(env, line);
		if (item)
			printf("%s=%s\n", line, item->value);
	}
	return 0;
}
