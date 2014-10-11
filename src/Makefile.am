bin_PROGRAMS=appjail
appjail_SOURCES=cap.c child.c main.c opts.c home.c mounts.c command.c network.c configfile.c tty.c x11.c path.c devpts.c run.c clone.c list.c list_helpers.c mask.c common.c fd.c wait.c notify.c redirect.c initstub.c env.c appjail.c
AM_CFLAGS=-Wall -DAPPJAIL_VERSION=\"$(APPJAIL_VERSION)\" -DAPPJAIL_SWAPDIR=\"$(datarootdir)/appjail\" -DAPPJAIL_CONFIGFILE=\"$(sysconfdir)/appjail.conf\" -DAPPLICATION_NAME=\"appjail\"
appjail_CFLAGS=$(AM_CFLAGS) $(libmount_CFLAGS) $(libcap_CFLAGS) $(libnl_CFLAGS) $(glib2_CFLAGS)
appjail_LDADD=$(libmount_LIBS) $(libcap_LIBS) $(libnl_LIBS) $(glib2_LIBS)
