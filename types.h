#ifndef H_TYPES
#define H_TYPES

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef char Char;
typedef Char* Str;
typedef ssize_t Index;
typedef _Bool Bool;
typedef pid_t Pid;
typedef struct termios Termios;
typedef int Fd;
typedef int Status;

#define when(x) break; case x
#define orwhen : case
#define otherwise break; default

#endif
