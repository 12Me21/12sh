#include <readline/readline.h>
#include <readline/history.h>
#include <term.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#include "types.h"
#include "dict.h"
#include "parse.h"
#include "run.h"
extern Str* environ;

Dict* env;

Str strcpy2(Str dest, Str src) {
	while (*dest++ = *src++);
	return dest;
}

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
	static Char prompt[1024];
	Str promptPos = prompt;
	Str ps1 = Dict_get(env, "PS1")->value;
	Str* args = parseLine(ps1);
	for (; *args; args++) {
		Str arg = *args;
		for (; *arg; arg++) {
			char c = *arg;
			if (c=='%') {
				c = *++arg;
				switch (c) {
				when('\0'):
					goto end;
				when('%'):
					*promptPos++ = '%';
				when('['):
					*promptPos++ = RL_PROMPT_START_IGNORE;
				when(']'):
					*promptPos++ = RL_PROMPT_END_IGNORE;
				when('$'):
					*promptPos++ = '$';
				otherwise:
					*promptPos++ = '%';
					*promptPos++ = c;
				}
			} else {
				*promptPos++ = c;
			}
		}
	end:;
	}
 
	/*Str pwd = Dict_get(env, "PWD")->value;
	Str user = Dict_get(env, "USER")->value;
	snprintf(prompt, sizeof(prompt)-1, "%c%s%c%s%c%s%c:%c%s%c%s%c%s%c$%c%s%c ",
		RL_PROMPT_START_IGNORE, "\033[0;1;38;5;22m", RL_PROMPT_END_IGNORE,
		user,
		RL_PROMPT_START_IGNORE, "\033[m", RL_PROMPT_END_IGNORE,
		RL_PROMPT_START_IGNORE, "\033[1;34m", RL_PROMPT_END_IGNORE,
		pwd,
		RL_PROMPT_START_IGNORE, "\033[m", RL_PROMPT_END_IGNORE,
		RL_PROMPT_START_IGNORE, "\033[m", RL_PROMPT_END_IGNORE
		);*/
	return prompt;
}
// "\[\033[0;1;38;5;22m]\]\u\[\033[m\]:\[\033[1;34m\]\w\[\033[m\]\$ "

void cursor_position(int* x, int* y) {
	static Str query;
	if (!query)
		query = tigetstr("u7");
	if (!query) {
		*x = *y = -1;
		return;
	}

	struct termios attr, backup;
	tcgetattr(0, &attr);
	backup = attr;
	/* Disable ICANON, ECHO, and CREAD. */
	attr.c_lflag &= ~(ICANON | ECHO | CREAD);
	tcsetattr(0, TCSAFLUSH, &attr);

	/* Request cursor coordinates from the terminal. */
	fputs(query, stdout);
	scanf("\033[%d;%dR", x, y); //todo: u6

	tcsetattr(0, TCSAFLUSH, &backup);

	// TODO:
	// set terminal settings to something sane before printing prompt
}

Str setf, setb;
void color(int f, int b) {
	putp(tiparm(setf, f));
	putp(tiparm(setb, b));
}

// called before readline prompt is displayed
// ensures that the cursor is in column 1
// prints \n if the last program's output didn't leave the cursor in col 1
int beforePrompt(void) {
	static Str op;
	if (!op)
		op = tigetstr("op");
	int y, x;
	cursor_position(&y, &x);
	if (x>1) {
		color(3+8, 0);
		putchar('%');
		putp(op);
		putchar('\n');
	}
	putchar('\r');
	return 0;
}

int main(int argc, Str* argv) {
	(void)argc;
	(void)argv;
	env = makeEnvDict(environ);
	setupterm(NULL, 1, NULL);
	rl_startup_hook = beforePrompt;
	setf = tigetstr("setaf");
	setb = tigetstr("setab");
	Dict_add(env, "PS1")->value = "%[\033[0;1;38;5;22m%]$USER%[\033[m%]:%[\033[1;34m%]$PWD%[\033[m%]%\\$\\ ";
	while (1) {
		// make sure cursor didn't end up at the start of a row
		Str line = readline(updatePrompt(env));
		if (!line)
			break;
		add_history(line);
		Str* args = parseLine(line);
		if (args[0]) {
			if (!strcmp(args[0], "unset")) {
				Dict_remove(env, args[1]);
			} else {
				Str path;
				int err = lookupCommand(args[0], Dict_get(env, "PATH")->value, &path);
				switch (err) {
				when(0):
					execute(path, args);
				otherwise:
					puts("oh no!");
				}
			}
		}
	}
	printf("\n");
	return 0;
}
