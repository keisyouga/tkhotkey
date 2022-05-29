
CFLAGS = -Wall -DUSE_TCL_STUBS -DUSE_TK_STUBS `pkg-config --cflags tk` -fPIC
LDLIBS =  `pkg-config --libs tk` -lX11
LDFLAGS = -shared -o lib$(PROGRAM).so
PROGRAM = hotkey
OBJS = $(PROGRAM).o

lib$(PROGRAM).so: $(OBJS)
	$(CC) $(LDFLAGS) $< $(LDLIBS)
