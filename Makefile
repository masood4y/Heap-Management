CFLAGS=-g
LIBS=lib212alloc.a

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

cpen212trace: cpen212alloc.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean
clean:
	$(RM) *.o cpen212trace
