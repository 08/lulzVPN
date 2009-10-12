VERSION	= 0.0.1
NAME	= lulzNet

CC	= gcc
CFLAGS	= -Wall -Wextra -pedantic -g -I ./include
LDFLAGS	= -lpthread -lssl -lreadline -lcrypt

LULZNET_FILES = src/auth.c src/config.c src/main.c src/peer.c\
		src/shell.c src/xfunc.c src/log.c src/networking.c\
		src/protocol.c src/tap.c src/packet.c

UG_FILES = var/lulznet_ug.c

all:lulznet ug
	gcc ${LDFLAGS} -o ${NAME} $(LULZNET_FILES:.c=.o)
	gcc ${LDFLAGS} -o ug $(UG_FILES:.c=.o) 


lulznet: $(LULZNET_FILES:.c=.o)

$(LULZNET_FILES:.c=.o): $(LULZNET_FILES)
	${CC} ${CFLAGS} ${INCLUDE} -o $*.o -c $*.c

ug: $(UG_FILES:.c=.o)

$(UG_FILES:.c=.o): $(UG_FILES)
	${CC} ${CFLAGS} ${INCLUDE} -o $*.o -c $*.c

indent:
	@indent src/*.c
	@rm src/*~

clean:
	rm src/*.o
	rm var/*.o
	rm ${NAME}
	rm ug
