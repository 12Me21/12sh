#include <readline/readline.h>
#include <readline/history.h>
#include <term.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <limits.h>

#include "types.h"
#include "dict.h"
#include "parse.h"

extern Str* environ;

/*char prompt[] = {
	't', 'e', 's', 't', RL_PROMPT_START_IGNORE,
	'\x1B', '[', '3', '1', 'm', RL_PROMPT_END_IGNORE,
	'$', RL_PROMPT_START_IGNORE,
	'\x1B', '[', 'm', RL_PROMPT_END_IGNORE,
	' ', '\0',
	};//*/

Char prompt[1024];

Dict* env;

void freeEnv(Str* envp) {
	for (; *envp; envp++)
		free(*envp);
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

Str* makeEnvCopy(Dict* env) {
	Str* envp = malloc(sizeof(Str*) * (env->items+1));
	Str* envpPos = envp;
	DictNode* item = env->shead;
	for (; item; (item=item->snext),envpPos++) {
		Index len1 = strlen(item->key);
		Index len2 = strlen(item->value);
		Str var = malloc(len1+1+len2+1);
		memcpy(var, item->key, len1);
		var[len1] = '=';
		memcpy(var+len1+1, item->value, len2);
		var[len1+1+len2] = '\0';
		*envpPos = var;
	}
	*envpPos = NULL;
	return envp;
}

Str updatePrompt(Dict* env) {
	Str pwd = Dict_get(env, "PWD")->value;
	Str user = Dict_get(env, "USER")->value;
	snprintf(prompt, sizeof(prompt)-1, "%c%s%c%s%c%s%c:%c%s%c%s%c%s%c$%c%s%c ",
		RL_PROMPT_START_IGNORE, "\033[0;1;38;5;22m", RL_PROMPT_END_IGNORE,
		user,
		RL_PROMPT_START_IGNORE, "\033[m", RL_PROMPT_END_IGNORE,
		RL_PROMPT_START_IGNORE, "\033[1;34m", RL_PROMPT_END_IGNORE,
		pwd,
		RL_PROMPT_START_IGNORE, "\033[m", RL_PROMPT_END_IGNORE,
		RL_PROMPT_START_IGNORE, "\033[m", RL_PROMPT_END_IGNORE
	);
	return prompt;
}

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
	return 0;
}

/**
 * search_path - searches path for program
 * @command: the command
 * @env_path: PATH
 * @filepath_out: pointer to file path output
 * Return: 0 (ok), 1 (file not found), 2 (permission error)
 */
int lookupCommand(Str command, Str env_path, Str* filepath_out) {
	static char filepath[PATH_MAX];
	int found_file = 0;

	// 0 = success
	int checkFile(Str filepath) {
		if (!access(filepath, F_OK)) { //exists
			found_file = 1;
			if (!access(filepath, X_OK)) { //executable
				*filepath_out = filepath;
				return 0;
			}
		}
		return 1;
	}
	
	 /* If command contains a slash, don't check PATH */
	if (strchr(command, '/'))	{
		if (!checkFile(command))
			return 0;
	} else {
		char *start = env_path;
		for (; *env_path; env_path++)
			if (*env_path == ':' || *env_path == '\0') {
				Index path_length = env_path - start;
				memcpy(filepath, start, path_length);
				// empty path item = current dir (relative)
				if (path_length)
					filepath[path_length++] = '/';
				strcpy(filepath + path_length, command);
				if (!checkFile(filepath))
					return 0;
				start = env_path + 1;
			}
	}
	return 1 + found_file;
}

int execute(Str path, Str* args) {
	Str* envp = makeEnvCopy(env);

	pid_t parchild = fork();
	if (parchild < 0)
		return (-1);

	int status;
	if (parchild == 0) {
		status = execve(path, args, envp);
		_exit(status);
	}
	
	wait(&status);
	freeEnv(envp);

	return WEXITSTATUS(status);
}

int main(int argc, Str* argv) {
	(void)argc;
	(void)argv;
	env = makeEnvDict(environ);
	setupterm(NULL, 1, NULL);
	rl_startup_hook = beforePrompt;
	setf = tigetstr("setaf");
	setb = tigetstr("setab");
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
