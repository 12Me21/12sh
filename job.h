#include "types.h"

typedef struct Process {
	struct Process* next; // next process in pipeline
	Str* argv; // for exec
	Pid pid;
	Bool completed;
	Bool stopped;
	Status status;
} Process;

typedef struct Job {
	struct Job* next;           /* next active job */
	Str command;              /* command line, used for messages */
	Process* first_process;     /* list of processes in this job */
	Pid pgid;                 /* process group ID */
	Bool notified;              /* true if user told about stopped job */
	Termios tmodes;      /* saved terminal modes */
	Fd stdin, stdout, stderr;  /* standard i/o channels */
} Job;

Termios shellTmodes;
Pid shellPgid;
extern Job* firstJob;
Bool shellInteractive;

Job* simpleJob(Str* argv);
void launchJob(Job* j, Bool foreground);
void doJobNotification(void);
void updateStatus(void);
void initShell(void);
void continueJob(Job* j, Bool foreground);
