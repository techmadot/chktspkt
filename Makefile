
CC = g++
#CFLAGS = -g
CFLAGS = -O2 
LDFLAGS = 
OBJS = main.o chktspkt.o tsselect.o
PROGRAM = chktspkt
DEST = /usr/local/bin

$(PROGRAM): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

.cpp.o:
	$(CC) $(CFLAGS) -c $<
.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(PROGRAM) 

clean:
	rm $(OBJS) $(PROGRAM) 

install:$(PROGRAM1)
	install -s $(PROGRAM) $(DEST)

tspacketchk.o: def.h
tsselect.o:  def.h
main.o:   def.h
chktspkt.o: def.h
