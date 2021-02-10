#pragma once
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

#define CRITICAL ?:(perror(NULL), exit(1), NULL)

#define ALLOC(name) name = malloc(sizeof(*name)) CRITICAL
#define ALLOCN(name, size) name = calloc(size, sizeof(*name)) CRITICAL
#define ALLOCI(name, init...) ALLOC(name); *name = (typeof(*name)){init}
#define ALLOCE(type) (malloc(sizeof(type)) CRITICAL)
#define ALLOCEN(type, size) (calloc(size, sizeof(type)) CRITICAL)
#define ALLOCEI(type, init...) ({type* ALLOCI(temp, init); temp;})

//#define ALLOCNI(name, size, init...) ALLOCN(name, size); *name = (typeof(*name)[size]){init}
