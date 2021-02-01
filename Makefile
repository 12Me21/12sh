srcs:= main dict parse run
output:= 12sh

libs:= readline curses

CFLAGS+= -Wextra -Wall -g -ftabstop=3

include Nice.mk
