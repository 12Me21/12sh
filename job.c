//reference: https://www.gnu.org/software/libc/manual/html_node/Data-Structures.html
#include "job.h"
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

Termios shellTmodes;
Pid shellPgid;
Job* firstJob = NULL;
Bool shellInteractive;
Fd ignore;

Job* simpleJob(CommandLine* cmd) {
	Job* ALLOCI(job,
		.next = firstJob,
		.command = strdup(cmd->command) CRITICAL,
		.pgid = 0,
		.notified = false,
		.stdin = 0,
		.stdout = 1,
		.stderr = 2,
	);
	Process* prev = NULL;
	for (; cmd; cmd=cmd->next) {
		Process* ALLOCI(process,
			.next= NULL,
			.argv= cmd->argv,
			.pid= 0,
			.completed = 0,
			.stopped = 0,
			.status = 0,
		);
		if (!prev)
			job->first_process = process;
		else
			prev->next = process;
		prev = process;
	}
	tcgetattr(0, &job->tmodes);
	firstJob = job;
	return job;
}

void freeJob(Job* j) {
	if (j == firstJob) {
		firstJob = j;
	} else {
		Job* s = firstJob;
		Job* prev = NULL;
		for (; s; s=s->next) {
			if (s==j) {
				prev->next = j->next;
				goto free;
			}
			prev = s;
		}
		return;
	}
 free:
	free(j);
}

Job* findJob(Pid pgid) {
	Job* j;
	for (j=firstJob; j; j=j->next)
		if (j->pgid == pgid)
			return j;
	return NULL;
}

Bool jobStopped(Job* j) {
	Process* p;
	for (p=j->first_process; p; p=p->next)
		if (!p->completed && !p->stopped)
			return false;
	return true;
}

/* Return true if all processes in the job have completed.  */
Bool jobCompleted(Job* j) {
	Process* p;
	for (p=j->first_process; p; p=p->next)
		if (!p->completed)
			return false;
	return true;
}

/* Store the status of the process pid that was returned by waitpid.
   Return 0 if all went well, nonzero otherwise.  */
int markProcessStatus(Pid pid, Status status) {
	Job* j;
	Process* p;

	if (pid > 0) {
		/* Update the record for the process.  */
		for (j=firstJob; j; j=j->next) {
			//dprintf(2, "mark process status, %s\n", j->command);
			for (p=j->first_process; p; p=p->next)
				if (p->pid == pid) {
					
					p->status = status;
					if (WIFSTOPPED(status))
						p->stopped = true;
					else {
						p->completed = true;
						if (WIFSIGNALED(status))
							fprintf (stderr, "%d: Terminated by signal %d.\n", (int)pid, WTERMSIG(p->status));
					}
					return 0;
				}
		}
		fprintf (stderr, "No child process %d.\n", pid);
		return -1;
	} else if (pid == 0 || errno == ECHILD)
		/* No processes ready to report.  */
		return -1;
	else {
		/* Other weird errors.  */
		perror ("waitpid");
		return -1;
	}
}

/* Check for processes that have status information available,
   blocking until all processes in the given job have reported.  */
void waitForJob(Job* j) {
	Status status;
	Pid pid;
	do {
		pid = waitpid(WAIT_ANY, &status, WUNTRACED);
	} while (!markProcessStatus(pid, status)
		&& !jobStopped(j)
		&& !jobCompleted(j));
}

/* Put a job in the background.  If the cont argument is true, send
   the process group a SIGCONT signal to wake it up.  */

void putJobInBackground(Job* j, Bool cont) {
  /* Send the job a continue signal, if necessary.  */
	if (cont)
		if (kill(-(j->pgid), SIGCONT) < 0)
			perror("kill (SIGCONT)");
}

void putJobInForeground(Job* j, Bool cont) {
	// Put the job into the foreground.
	tcsetpgrp(0, j->pgid);

	// Send the job a continue signal, if necessary.
	if (cont) {
		tcsetattr(0, TCSADRAIN, &j->tmodes);
		if (kill(-(j->pgid), SIGCONT) < 0)
			perror("kill (SIGCONT)");
	}
	
	// Wait for it to report.
	waitForJob(j);
	
	// Put the shell back in the foreground.
	tcsetpgrp(0, shellPgid);

	// Restore the shell’s terminal modes.
	tcgetattr(0, &j->tmodes);
	tcsetattr(0, TCSADRAIN, &shellTmodes);
}

void launchProcess(Process* p, Pid pgid, Fd infile, Fd outfile, Fd errfile, Bool foreground) {
	Pid pid;
	if (shellInteractive) {
		// Put the process into the process group and give the process group the terminal, if appropriate.
		// This has to be done both by the shell and in the individual child processes because of potential race conditions.
		pid = getpid();
		if (pgid == 0) pgid = pid;
		setpgid(pid, pgid);
		if (foreground)
			tcsetpgrp(0, pgid);
		// Set the handling for job control signals back to the default.
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);
		signal(SIGTTIN, SIG_DFL);
		signal(SIGTTOU, SIG_DFL);
		signal(SIGCHLD, SIG_DFL);
	}

	//if (foreground) {
		// Set the standard input/output channels of the new process.
		if (infile != STDIN_FILENO) {
			dup2(infile, STDIN_FILENO);
			close(infile);
		}
		if (outfile != STDOUT_FILENO) {
			dup2(outfile, STDOUT_FILENO);
			close(outfile);
		}
		if (errfile != STDERR_FILENO) {
			dup2(errfile, STDERR_FILENO);
			close(errfile);
		}
		/*	} else {
		dup2(ignore, STDIN_FILENO);
		dup2(ignore, STDOUT_FILENO);
		dup2(ignore, STDERR_FILENO);
		}*/

	// Exec the new process.  Make sure we exit.
	execvp(p->argv[0], p->argv);
	perror("execvp");
	exit(1);
}

// Format information about job status for the user to look at.
void formatJobInfo(Job* j, const Str status) {
	fprintf(stderr, "%ld (%s): %s\n", (long)j->pgid, status, j->command);
}

void launchJob(Job* j, Bool foreground) {
	Process* p;
	Pid pid;
	Fd mypipe[2], infile, outfile;
	
	infile = j->stdin;
	for (p=j->first_process; p; p=p->next) {
		// Set up pipes, if necessary.
		if (p->next) {
			if (pipe(mypipe) < 0) {
				perror("pipe");
				exit(1);
			}
			outfile = mypipe[1];
		} else
			outfile = j->stdout;

		// Fork the child processes.
		pid = fork();
		if (pid == 0)
			// This is the child process.
			launchProcess(p, j->pgid, infile, outfile, j->stderr, foreground);
		else if (pid < 0) {
			// The fork failed.
			perror("fork");
			exit(1);
		} else {
			// This is the parent process.
			p->pid = pid;
			if (shellInteractive) {
				if (!j->pgid)
					j->pgid = pid;
				setpgid(pid, j->pgid);
			}
		}
		
		// Clean up after pipes.
		if (infile != j->stdin)
			close(infile);
		if (outfile != j->stdout)
			close(outfile);
		infile = mypipe[0];
	}
	
	//formatJobInfo(j, "launched");

	if (!shellInteractive)
		waitForJob(j);
	else if (foreground)
		putJobInForeground(j, 0);
	else
		putJobInBackground(j, 0);
}

// Check for processes that have status information available, without blocking.
void updateStatus(void) {
	Status status;
	Pid pid;
	do {
		pid = waitpid(WAIT_ANY, &status, WUNTRACED|WNOHANG);
	} while (!markProcessStatus(pid, status));
}

/* Notify the user about stopped or terminated jobs.
   Delete terminated jobs from the active job list.  */
void doJobNotification (void) {
	Job* j, *jnext;
	
	/* Update status information for child processes.  */
	updateStatus();
	
	Job* jlast = NULL;
	for (j=firstJob; j; j=jnext) {
		jnext = j->next;
		//printf("checking %s %d %d\n", j->command, j->pgid, j->first_process->pid);
		/* If all processes have completed, tell the user the job has
         completed and delete it from the list of active jobs.  */
		if (jobCompleted(j)) {
			//formatJobInfo(j, "completed");
			if (jlast)
				jlast->next = jnext;
			else
				firstJob = jnext;
			freeJob(j);
		} else if (jobStopped (j) && !j->notified) {
			/* Notify the user about stopped jobs,
			   marking them so that we won’t do this more than once.  */
			formatJobInfo (j, "stopped");
			j->notified = 1;
			jlast = j;
		} else {
			formatJobInfo (j, "running");
			/* Don’t say anything about jobs that are still running.  */
			jlast = j;
		}
	}
}

void markJobAsRunning(Job* j) {
	Process* p;
	for (p=j->first_process; p; p=p->next)
		p->stopped = 0;
	j->notified = 0;
}

/* Continue the job J.  */
void continueJob(Job* j, Bool foreground) {
	markJobAsRunning(j);
	if (foreground)
		putJobInForeground(j, 1);
	else
		putJobInBackground(j, 1);
}

void initShell(void) {
	shellInteractive = isatty(0);
	if (shellInteractive) {
		// check if shell is in foreground
		// and if not, send SIGTTIN to self
		// to wait until it's placed in the fg
		while (tcgetpgrp(0) != (shellPgid = getpgrp()))
			kill(-shellPgid, SIGTTIN);
		// ignore interactive + job control signals
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
		signal(SIGTTIN, SIG_IGN);
		signal(SIGTTOU, SIG_IGN);
		signal(SIGCHLD, SIG_IGN);
		// put ourselves in our own process group
		shellPgid = getpid();
		if (setpgid(shellPgid, shellPgid) < 0) {
			perror("Couldn't put the shell in its own process group");
			exit(1);
		}
		// take control of terminal
		tcsetpgrp(0, shellPgid);
		// save terminal attributes
		tcgetattr(0, &shellTmodes);
	}
	ignore = open("/dev/null", O_RDWR) CRITICAL;
}
