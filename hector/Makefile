# builds the tracker server
# rodrigo: add your .c files to SRC as needed

CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lpthread -lssl -lcrypto

# mac openssl fix (linux ignores this)
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    OPENSSL := $(shell brew --prefix openssl 2>/dev/null)
    ifneq ($(OPENSSL),)
        CFLAGS += -I$(OPENSSL)/include
        LDFLAGS += -L$(OPENSSL)/lib
    endif
endif

SRC = tracker.c

tracker: $(SRC)
	$(CC) $(CFLAGS) -o tracker $(SRC) $(LDFLAGS)

clean:
	rm -f tracker *.o
