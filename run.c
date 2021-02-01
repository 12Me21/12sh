#include <limits.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

#include "types.h"
#include "dict.h"

extern Dict* env;

static void freeEnv(Str* envp) {
	for (; *envp; envp++)
		free(*envp);
}

static Str* makeEnvCopy(Dict* env) {
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
