srcs:= main dict parse run term
output:= 12sh

libs:= readline curses

CFLAGS+= -Wextra -Wall -g -ftabstop=3 -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable

include Nice.mk
