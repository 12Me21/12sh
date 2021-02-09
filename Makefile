srcs:= main dict parse run term
output:= 12sh

libs:= readline curses

LD_LIBRARY_PATH=/usr/local/lib

CFLAGS+= -Wextra -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Werror=implicit-function-declaration -g -ftabstop=3

include Nice.mk
