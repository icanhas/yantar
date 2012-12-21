all: $(Q3ASM)

clean:
	$Qrm -f $(Q3ASM) $(Q3ASMOBJ)

Q3ASMOBJ = \
  q3asm.o \
  cmdlib.o

.c.o:
	$(echo_cmd) "TOOLS_CC $<"
	$Q$(CC) $(CFLAGS) $(INC) -o $@ -c $<

$(Q3ASM): $(Q3ASMOBJ)
	$(echo_cmd) LD $@
	$Q$(CC) $(TOOLS_CFLAGS) $(TOOLS_LDFLAGS) -o $@ $^ $(TOOLS_LIBS)
