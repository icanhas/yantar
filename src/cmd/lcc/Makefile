INC =  \
  -Isrc \
  -Ilburg

LBURGOBJ = \
  lburg/lburg.o \
  lburg/gram.o
	
LCCOBJ = \
  src/alloc.o \
  src/bind.o \
  src/bytecode.o \
  src/dag.o \
  src/dagcheck.o \
  src/decl.o \
  src/enode.o \
  src/error.o \
  src/event.o \
  src/expr.o \
  src/gen.o \
  src/init.o \
  src/inits.o \
  src/input.o \
  src/lex.o \
  src/list.o \
  src/main.o \
  src/null.o \
  src/output.o \
  src/prof.o \
  src/profio.o \
  src/simp.o \
  src/stmt.o \
  src/string.o \
  src/sym.o \
  src/symbolic.o \
  src/trace.o \
  src/tree.o \
  src/types.o
  
Q3CPPOBJ = \
  cpp/cpp.o \
  cpp/eval.o \
  cpp/getopt.o \
  cpp/hideset.o \
  cpp/include.o \
  cpp/lex.o \
  cpp/macro.o \
  cpp/nlist.o \
  cpp/tokens.o \
  cpp/unix.o

Q3LCCOBJ = \
  etc/lcc.o \
  etc/bytecode.o
  
all: $(Q3LCC) 

clean:
	$Qrm -f $(RCC) $(LBURG) $(Q3CPP) $(Q3LCC)
	$Qrm -f $(LCCOBJ) $(LBURGOBJ) $(Q3CPPOBJ) $(Q3LCCOBJ) $(DAGCHECK_C)

$(LBURG): $(LBURGOBJ)
	$(echo_cmd) "LD $@"
	$Q$(CC) $(CFLAGS) $(TOOLS_LDFLAGS) -o $@ $^ $(TOOLS_LIBS)

$(DAGCHECK_C): $(LBURG) src/dagcheck.md
	$(echo_cmd) "LBURG src/dagcheck.md"
	$Q$(LBURG) src/dagcheck.md $@

src/dagcheck.o: $(DAGCHECK_C)
	$(echo_cmd) "TOOLS_CC_DAGCHECK $<"
	$Q$(CC) $(CFLAGS) $(INC) -Wno-unused -o $@ -c $<

rcc/%.o: src/%.c
	$(echo_cmd) "TOOLS_CC $<"
	$Q$(CC) $(CFLAGS)  $(INC) -o $@ -c $<

$(RCC): $(LCCOBJ)
	$(echo_cmd) "LD $@"
	$Q$(CC) $(CFLAGS) $(TOOLS_LDFLAGS) -o $@ $^ $(TOOLS_LIBS)

$(Q3CPP): $(Q3CPPOBJ)
	$(echo_cmd) "LD $@"
	$Q$(CC) $(CFLAGS) $(TOOLS_LDFLAGS) -o $@ $^ $(TOOLS_LIBS)

.c.o:
	$(echo_cmd) "TOOLS_CC $<"
	$Q$(CC) $(CFLAGS)  $(INC) -o $@ -c $<

$(Q3LCC): $(RCC) $(Q3CPP) $(Q3LCCOBJ) 
	$(echo_cmd) "LD $@"
	$Q$(CC) $(CFLAGS) $(TOOLS_LDFLAGS) -o $@ $(Q3LCCOBJ) $(TOOLS_LIBS)
