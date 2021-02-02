#define _DEFAULT_SOURCE 10
#include <locale.h>
#include <term.h>
#include <stdio.h>
#include "types.h"
#include "term.h"

static Str setaf, setab, op, u7, cr, cnorm;

void terminfo_init(void) {
	setlocale(LC_ALL, "");
	setupterm(NULL, 1, NULL);
	setaf = tigetstr("setaf");
	setab = tigetstr("setab");
	op = tigetstr("op");
	u7 = tigetstr("u7");
	cr = tigetstr("cr");
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

static void makeSane() {
	struct termios attr;
	tcgetattr(0, &attr);
	attr.c_iflag |= BRKINT | ICRNL | +IXANY;
	attr.c_iflag &= ~(IGNBRK | INLCR | IGNCR | IXOFF | IUTF8 | IUCLC);
	attr.c_oflag |= OPOST | ONLCR | NL0 | CR0 | TAB0 | BS0 | VT0 | FF0;
	attr.c_oflag &= ~(OLCUC | OCRNL | OFILL | ONOCR | ONLRET);
	attr.c_lflag |= ICANON | IEXTEN | ECHO | ECHOE | ECHOK | IMAXBEL | ISIG | ECHOKE;
	attr.c_lflag &= ~(ECHONL | NOFLSH | XCASE | TOSTOP | OFDEL | ECHOPRT | FLUSHO | +ECHOCTL);
	attr.c_cflag |= CREAD ;
	//attr.c_cflag &= ~();
	//-extproc
	
	tcsetattr(0, TCSAFLUSH, &attr);

	putp(tigetstr("cnorm"));
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
