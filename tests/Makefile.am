TESTS = check_libflm
check_PROGRAMS = check_libflm
check_libflm_SOURCES = 	main.c				\
						alloc_test.c 		\
						buffer_test.c 		\
						epoll_test.c		\
						monitor_test.c		\
						timer_test.c		\
						thread_test.c		\
						io_test.c		    \
						stream_test.c	    \
						test_utils.c

check_libflm_CFLAGS = @CHECK_CFLAGS@ -W -Wall -I../include/
check_libflm_LDADD = $(top_builddir)/src/libflm.la @CHECK_LIBS@

check:
	mv -f ../src/.libs/*.gcno ../src/.libs/*.gcda ../src/. ; \
	cd ../src && gcov *.gcno >& /dev/null && echo "gcov output files created"
	rm -f ../src/*.gcno
	rm -f ../src/*.gcda
