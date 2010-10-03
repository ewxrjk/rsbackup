prefix=/usr/local
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
mandir=${prefix}/man
VERSION=0.0
PODCMD=pod2man -c rsbackup -r "version ${VERSION}"

all: rsbackup.1 rsbackup.cron.1 rsbackup-mount.1 rsbackup.1.html rsbackup.cron.1.html rsbackup-mount.1.html
	perl -wc rsbackup

rsbackup.1: rsbackup
	podchecker $<
	${PODCMD} $< > $@

%.1: %.pod
	podchecker $<
	${PODCMD} $< > $@

%.1.html: %.1
	./htmlman -- $<

installdirs:
	mkdir -p ${bindir}
	mkdir -p ${mandir}/man1

install: installdirs
	install -m 555 rsbackup ${bindir}/rsbackup
	install -m 555 rsbackup.cron ${bindir}/rsbackup.cron
	install -m 555 rsbackup-mount ${bindir}/rsbackup-mount
	install -m 444 rsbackup.1 ${mandir}/man1/rsbackup.1
	install -m 444 rsbackup.cron.1 ${mandir}/man1/rsbackup.cron.1
	install -m 444 rsbackup-mount.1 ${mandir}/man1/rsbackup-mount.1

uninstall:
	rm -f ${bindir}/rsbackup
	rm -f ${bindir}/rsbackup.cron
	rm -f ${mandir}/man1/rsbackup.1
	rm -f ${mandir}/man1/rsbackup.cron.1
	rm -f ${mandir}/man1/rsbackup-mount.1

dist:
	rm -rf rsbackup-${VERSION}
	mkdir -p rsbackup-${VERSION}/debian
	cp README rsbackup-${VERSION}
	cp COPYING rsbackup-${VERSION}
	cp Makefile rsbackup-${VERSION}
	cp rsbackup rsbackup-${VERSION}
	cp rsbackup-mount rsbackup-${VERSION}
	cp rsbackup.cron rsbackup-${VERSION}
	cp rsbackup.cron.pod rsbackup-${VERSION}
	cp rsbackup-mount.pod rsbackup-${VERSION}
	cp htmlman rsbackup-${VERSION}
	cp rsbackup.html rsbackup-${VERSION}
	cp debian.html rsbackup-${VERSION}
	cp rsbackup.css rsbackup-${VERSION}
	cp disk-encryption.html rsbackup-${VERSION}
	cp rsbackup.hourly rsbackup-${VERSION}
	cp rsbackup.daily rsbackup-${VERSION}
	cp rsbackup.weekly rsbackup-${VERSION}
	cp rsbackup.monthly rsbackup-${VERSION}
	cp rsbackup.defaults rsbackup-${VERSION}
	cp rsbackup.config rsbackup-${VERSION}
	cp rsbackup.devices rsbackup-${VERSION}
	cp debian/changelog rsbackup-${VERSION}/debian
	cp debian/copyright rsbackup-${VERSION}/debian
	cp debian/control rsbackup-${VERSION}/debian
	cp debian/rsbackup.conffiles rsbackup-${VERSION}/debian
	cp debian/rsbackup.postinst rsbackup-${VERSION}/debian
	cp debian/rules rsbackup-${VERSION}/debian
	tar cf rsbackup-${VERSION}.tar rsbackup-${VERSION}
	gzip -9f rsbackup-${VERSION}.tar
	rm -rf rsbackup-${VERSION}

clean:
	rm -f rsbackup.1 rsbackup.cron.1 rsbackup-mount.1
	rm -f rrsbackup.1.html rsbackup.cron.1.html rsbackup-mount.1.html
