#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "types.h"
#include "dict.h"
#include "parse.h"
#include "run.h"
#include "term.h"
#include "job.c"
extern Str* environ;

Dict* env;

Str strcpy2(Str dest, Str src) {
	while ((*dest++ = *src++));
	return dest;
}

Str signalMsg = NULL;

void handleSignal(int num) {
	if (num==SIGINT)
		signalMsg = "SIGINT";
	else
		signalMsg = "signal?";
	if (num==SIGCHLD) {
		//puts("CHLD");
		//xdoJobNotification();
	}
}

Dict* makeEnvDict(Str* envp) {
	Dict* new = Dict_new(100);
	for (; *envp; envp++) {
		Str name, value;
		Str sep = strchr(*envp, '=');
		if (sep) {
			name = strndup(*envp, sep-*envp) CRITICAL;
			value = strdup(sep+1) CRITICAL;
		} else {
			name = strdup(*envp) CRITICAL;
			value = strdup("") CRITICAL; //eh
		}
		*(Str*)Dict_add(new, name) = value;
		free(name);
	}
	return new;
}

Str updatePrompt(Dict* env) {
	static Char prompt[1024];
	Str promptPos = prompt;
	Str ps1 = *(Str*)(Dict_get(env, "PS1")?:&"$ ");
	//Str* args = parseLine(ps1, false);
	Str arg = ps1;//args[0];
	for (; *arg; arg++) {
		char c = *arg;
		switch (c) {
		when('\1'):
			c = RL_PROMPT_START_IGNORE;
		when('\2'):
			c = RL_PROMPT_END_IGNORE;
		}
		*promptPos++ = c;
	}
	*promptPos++ = '\0';
	//freeArgs(args);
	
	return prompt;
}

Str getLine() {
	return readline(updatePrompt(env));
}

int main(int argc, Str* argv) {
	(void)argc;
	(void)argv;
	initShell();
	signal(SIGCHLD, SIG_DFL);
	
	struct sigaction new_action;
	new_action.sa_handler = handleSignal;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction(SIGINT, &new_action, NULL);
	sigaction(SIGHUP, &new_action, NULL);
	sigaction(SIGTSTP, &new_action, NULL);

	terminfo_init();
	env = makeEnvDict(environ);
	*(Str*)Dict_add(env, "PS1") = strdup("\1\033[0;1;38;5;22m\2$USER\1\033[m\2:\1\033[1;34m\2$PWD\1\033[m\2\\$\\ ") CRITICAL;
	
	while (1) {
		if (signalMsg) {
			puts(signalMsg);
			signalMsg = NULL;
		}
		// make sure cursor didn't end up at the start of a row
		beforePrompt();
		Str line = getLine();
		signalMsg = NULL;
		if (!line)
			break;
		add_history(line);
		CommandLine* cmd = parseLine(line);
		Str command = cmd->command;
		if (command) {
			if (!strcmp(command, "exit")) {
				break;
			} else if (!strcmp(command, "set")) {
				*(Str*)Dict_add(env, cmd->argv[1]) = strdup(cmd->argv[2]) CRITICAL;
			} else if (!strcmp(command, "fg")) {
				if (firstJob)
					putJobInForeground(firstJob, true);
			} else {
				Str path;
				int err = lookupCommand(command, *(Str*)(Dict_get(env, "PATH")?:&""), &path);
				switch (err) {
				when(0):;
					Job* job = simpleJob(cmd);
					launchJob(job, !cmd->bg);
					//execute(path, args);
				otherwise:
					printf("Command '%s' not found\n", command);
				}
			}
		}
		CommandLine_free(cmd);
		free(line);
		signal(SIGCHLD, SIG_IGN);
		doJobNotification();
		signal(SIGCHLD, SIG_DFL);
	}
	printf("exit\n");
	Dict_free(env);
	return 0;
}
