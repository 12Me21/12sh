CFLAGS?= -Wextra -Wall -g
# libs to include when linking (with -l<name> flags)
libs:= readline
# c source code files
srcs:= main
output:= 12sh
# location for intermediate files (.o and .mk)
junkdir:= .junk

################################################################

CC=@echo '[33m'$@'	[37mfrom: [32m'$^'[m' ; cc

$(output): $(srcs:%=$(junkdir)/%.o)
	$(CC) $(CFLAGS) $(addprefix -l,$(libs)) $^ -o $@

$(junkdir)/%.mk: %.c
	$(CC) $(CFLAGS) -MF$@ -MG -MM -MP -MT$@ -MT$(<:%.c=$(junkdir)/%.o) $<

$(junkdir)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Normally, Make will try to generate nonexistent included files (in this case, with our $(junkdir)/%.mk rule
# But for some reason, include fails if the file is in a nonexistent directory.
# This is the only solution I can think of:
$(shell mkdir -p $(addprefix $(junkdir)/,$(dir $(srcs))))
#PROBLEM: this includes files even if `clean` is being run, causing it to generate dependency files xd
include $(srcs:%=$(junkdir)/%.mk)

#clean:
#	$(RM) -r $(junkdir)
#	$(RM) $(output)

#.PHONY: clean
