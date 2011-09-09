TARGET = msp430_serial
VERSION = 0.0.1
DESTINATION = /usr/local/bin
HEADER = msp430_serial.h
CFLAGS += -fPIC -g -Wall
LIBFLAGS = -lusb-1.0
CC = gcc

%.o: %.c
	$(CC) $(CFLAGS) -c $<

all: $(TARGET)

$(TARGET): msp430_serial.o $(HEADER)
	$(CC) -o $(TARGET) msp430_serial.o $(LIBFLAGS) 

install: $(TARGET) 
	test -z $(DESTINATION) || mkdir -p $(DESTINATION)
	install $(TARGET) $(DESTINATION)

clean:
	rm -f $(TARGET) *.o *~