CC = gcc
CFLAGS = -O2 -Wall
CPPFLAGS = -I.
LDFLAGS =

TARGET = cpicsk_gen
ZIP = cpicsk_gen.zip

OBJS = cpicsk_gen.o
LIST=archive.lst

all: $(TARGET)

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) $(FLAGS) -c $<

cpicsk_gen: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -f *.o *~ $(TARGET)

archive: $(LIST)
	cd .. && zip -r $(ZIP) cpicsk_gen -i@cpicsk_gen/$(LIST)
