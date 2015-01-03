# commands
INSTALL := install
MD	:= markdown
RM	:= rm
CP	:= cp
# this is just a fallback in case you do not use git but downloaded
# a release tarball...
VERSION := 0.5.0

all: bin/ykfde bin/ykfde-cpio udev/ykfde README.html

bin/ykfde: bin/ykfde.c config.h
	$(MAKE) -C bin

bin/ykfde-cpio: bin/ykfde-cpio.c config.h
	$(MAKE) -C bin

udev/ykfde: udev/ykfde.c config.h
	$(MAKE) -C udev

config.h: config.def.h
	$(CP) config.def.h config.h

README.html: README.md
	$(MD) README.md > README.html

install: install-mkinitcpio

install-bin: bin/ykfde udev/ykfde
	$(MAKE) -C bin install
	$(MAKE) -C udev install
	$(INSTALL) -D -m0644 conf/ykfde.conf $(DESTDIR)/etc/ykfde.conf
	$(INSTALL) -D -m0644 systemd/ykfde-cpio.service $(DESTDIR)/usr/lib/systemd/system/ykfde-cpio.service
	$(INSTALL) -d -m0700 $(DESTDIR)/etc/ykfde.d/

install-doc: README.md README.html
	$(INSTALL) -D -m0644 README.md $(DESTDIR)/usr/share/doc/ykfde/README.md
	$(INSTALL) -D -m0644 README.html $(DESTDIR)/usr/share/doc/ykfde/README.html

install-mkinitcpio: install-bin install-doc
	$(INSTALL) -D -m0644 mkinitcpio/ykfde $(DESTDIR)/usr/lib/initcpio/install/ykfde
	$(INSTALL) -D -m0644 mkinitcpio/ykfde-cpio $(DESTDIR)/usr/lib/initcpio/install/ykfde-cpio

install-dracut: install-bin install-doc
	$(INSTALL) -d -m0755 $(DESTDIR)/usr/lib/dracut/modules.d/90ykfde/
	$(INSTALL) -t $(DESTDIR)/usr/lib/dracut/modules.d/90ykfde/ systemd/90ykfde/module-setup.sh systemd/90ykfde/ykfde.sh systemd/90ykfde/parse-mod.sh

clean:
	$(MAKE) -C bin clean
	$(MAKE) -C udev clean
	$(RM) -f README.html

release:
	git archive --format=tar.gz --prefix=mkinitcpio-ykfde-$(VERSION)/ $(VERSION) > mkinitcpio-ykfde-$(VERSION).tar.gz
	gpg -ab mkinitcpio-ykfde-$(VERSION).tar.gz
