bin_PROGRAMS=appjail
appjail_SOURCES=cap.c child.c main.c opts.c home.c propagation.c
AM_CFLAGS=-Wall
appjail_CFLAGS=$(libmount_CFLAGS) $(libcap_CFLAGS)
appjail_LDADD=$(libmount_LIBS) $(libcap_LIBS)
