lib_LTLIBRARIES = libflm.la

libflm_la_SOURCES =	\
alloc.c				\
buffer.c			\
error.c				\
file.c				\
io.c				\
monitor.c			\
epoll.c				\
select.c			\
obj.c				\
stream.c			\
tcp_server.c		\
thread.c			\
thread_pool.c		\
timer.c

#libflm_la_CFLAGS = -W -Wall -fprofile-arcs -ftest-coverage -O0 -I../ -I../include/ -ggdb
libflm_la_CFLAGS = -W -Wall -O2 -I../ -I../include/
libflm_la_LDFLAGS = -lm -lrt -lssl

clean-local:
	rm -f *.gcda
	rm -f *.gcno
