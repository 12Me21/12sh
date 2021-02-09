#define _DEFAULT_SOURCE 10
#include <locale.h>
#include <term.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include "types.h"
#include "term.h"

static Str setaf, setab, op, u7, cr, cnorm;

void redisplay(void) {
	rl_forced_update_display();
	rl_redisplay();
}

static void getCursorPosition(int* x, int* y) {
	if (!u7) {
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
	putp(u7);
	scanf("\033[%d;%dR", y, x); //todo: u6

	tcsetattr(0, TCSAFLUSH, &backup);
}

static void setColor(int f, int b) {
	putp(tiparm(setaf, f));
	putp(tiparm(setab, b));
}

static void resetColor(void) {
	putp(op);
}

int rl_tty_set_echoing(int value);

static void makeSane() {
	struct termios attr;
	tcgetattr(0, &attr);
	// c_iflag //
	attr.c_iflag |= BRKINT | ICRNL | +IXANY;
	attr.c_iflag &= ~(IGNBRK | INLCR | IGNCR | IXOFF | IUTF8 | IUCLC);
	// !IGNBRK + BRKINT -> BREAK sends SIGINT
	//-IGNPAR, PARMRK, INPCK, ISTRIP (unchanged)
	// !INLCR -> disable translating NL input to CR
	// !IGNCR -> disable ignoring CR input
	// ICRNL -> translate CR to NL input (I assume this means for echo?)
	// !IUCLC -> disable mapping uppercase to lowercase
	//-IXON (unchanged)
	// IXANY -> typing will start output after XOFF
	// !IXOFF -> disable XON/XOFF on input
	//-IMAXBEL (unchanged)
	// !IUTF8 -> not sure why this is disabled...

	// c_oflag //
	attr.c_oflag |= OPOST | ONLCR | NL0 | CR0 | TAB0 | BS0 | VT0 | FF0;
	attr.c_oflag &= ~(OLCUC | OCRNL | OFILL | ONOCR | ONLRET);
	// OPOST -> "implementation-defined output postprocessing"
	// !OLCUC -> disable map lowercase to uppercase
	// ONLCR -> map NL to CRNL on output
	// !OCRNL -> disable mapping CR to NL
	// !ONOCR -> disable not sending CR in column 0
	// !ONLRET -> enable outputting CR
	// !OFILL -> disable sending fill chars for delay
	//-OFDEL (unchanged)
	// NL0 -> newline delay 0 ?
	// CR0 -> CR delay 0 ?
	// TAB0 -> tab delay 0
	// BS0 -> backspace delay 0
	// VT0 -> vertical tab delay 0
	// FF0 -> form feed delay 0

	// c_cflag //
	attr.c_cflag |= CREAD;
	//attr.c_cflag &= ~();
	//-CBAUD, CBAUDEX, CSIZE, CSTOPB (unchanged)
	// CREAD -> enable reciever
	//-PARENB, PARODD, HUPCL, CLOCAL, LOBLK, CIBAUD, CMSPAR, CRTSCTS (unchanged)
	
	// c_lflag //
	attr.c_lflag |= ICANON | IEXTEN | ECHO | ECHOE | ECHOK | IMAXBEL | ISIG | ECHOKE | +TOSTOP;
	attr.c_lflag &= ~(ECHONL | NOFLSH | XCASE | ECHOPRT | FLUSHO | +ECHOCTL);
	// ISIG -> generate signals for C-c etc.
	// ICANON -> enable canonical mode
	//!XCASE -> disable uppercase-only
	// ECHO -> echo input chars
	// ECHOE -> ERASE erases char, WERASE erases word
	// ECHOK -> KILL erases line
	//!ECHONL -> echo NL even if ECHO is not set
	//!ECHOCTL -> disable echoing special chars
	//!ECHOPRT -> ?
	// ECHOKE -> ?
	//-DEFECHO (unchanged)
	//!FLUSHO -> output is not being flushed ?
	//!NOFLSH -> flush when generating signals
	// TOSTOP -> send SIGTTOU signal to bg process that DARES WRITE OUTPUT TO TERMINAL
	//!PENDIN (unchanged)
	// IEXTEN -> implementation defined input processing

	attr.c_cc[VSTOP] = _POSIX_VDISABLE;
	
	//-extproc
	
	tcsetattr(0, TCSAFLUSH, &attr);
	putp(tigetstr("cnorm"));
}

void terminfo_init(void) {
	setlocale(LC_ALL, "");
	setupterm(NULL, 1, NULL);
	setaf = tigetstr("setaf");
	setab = tigetstr("setab");
	op = tigetstr("op");
	u7 = tigetstr("u7");
	cr = tigetstr("cr");
	makeSane();
}

// called before readline prompt is displayed
// ensures that the cursor is in column 1
// prints \n if the last program's output didn't leave the cursor in col 1
int beforePrompt(void) {
	makeSane();
	int x, y;
	getCursorPosition(&x, &y);
	if (x>1) {
		setColor(3+8, 0);
		putchar('%');
		resetColor();
		putchar('\n');
	} else {
		resetColor();
	}
	putp(cr);
	return 0;
}
