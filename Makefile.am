SUBDIRS = src

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(datarootdir)/appjail
	$(MKDIR_P) $(DESTDIR)$(sysconfdir)
	$(INSTALL) -m644 appjail.conf $(DESTDIR)$(sysconfdir)/appjail.conf
