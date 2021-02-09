#include "types.h"

enum SourceType {Source_NONE, Source_FILE, Source_STRING};

// source or dest for > < redirection
typedef struct Source {
	enum SourceType type;
	union {
		Str filepath;
		Str string;
	};
	Fd fd;
	mode_t mode;
} Source;

typedef struct CommandLine {
	Str command;
	Str* argv;
	struct CommandLine* next;
	Bool bg;
	Bool pipeout;
	Source* redirections;
} CommandLine;

Str* parseLine(Str, Bool);
void freeArgs(Str*);
