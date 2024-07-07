CC=gcc
CFLAGS=-c -Wall -Werror
SOURCES=test_chardevice.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=test_chardevice

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
