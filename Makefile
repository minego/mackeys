TARGET			= mackeys
INSTALL_DIR		:= /opt/interception/
SRC				= ${TARGET}.c
OBJ				= ${SRC:.c=.o}
CFLAGS			?= -O0 -g -ggdb -Wall

all: ${TARGET}

.c.o:
	${CC} -c ${CFLAGS} $<

${TARGET}: ${OBJ}
	${CC} ${CFLAGS} -o $@ ${OBJ} ${LDFLAGS}

.PHONY: clean
clean:
	rm -f ${TARGET} ${OBJ}

.PHONY: install
install:
	# install -D --strip -T ${TARGET} ${INSTALL_DIR)/${TARGET}
	install -D -T ${TARGET} ${INSTALL_DIR}/${TARGET}

