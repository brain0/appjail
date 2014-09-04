bin_PROGRAMS=appjail
appjail_SOURCES=cap.c child.c main.c opts.c home.c propagation.c command.c network.c
AM_CFLAGS=-Wall -DAPPJAIL_SWAPDIR=\"$(datarootdir)/appjail\"
appjail_CFLAGS=$(AM_CFLAGS) $(libmount_CFLAGS) $(libcap_CFLAGS) $(libnl_CFLAGS)
appjail_LDADD=$(libmount_LIBS) $(libcap_LIBS) $(libnl_LIBS)
