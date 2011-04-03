prefix=/usr/local
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
mandir=${prefix}/man
datarootdir = ${prefix}/share
docdir = ${datarootdir}/doc/rsbackup
VERSION=0.1
PODCMD=pod2man -c rsbackup -r "version ${VERSION}"
MANPAGES=rsbackup.1 rsbackup.cron.1 rsbackup-mount.1
HTMLMAN=rsbackup.1.html rsbackup.cron.1.html rsbackup-mount.1.html
HTMLDOC=debian.html disk-encryption.html rsbackup.html rsbackup.css
PROGS=rsbackup rsbackup.cron rsbackup-mount

all: ${MANPAGES} ${HTMLMAN}
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
	mkdir -p ${DESTDIR}${bindir}
	mkdir -p ${DESTDIR}${mandir}/man1

install: installdirs
	install -m 755 ${PROGS} ${DESTDIR}${bindir}/.
	install -m 644 ${MANPAGES} ${DESTDIR}${mandir}/man1/.

install-doc:
	mkdir -p ${DESTDIR}${docdir}
	install -m 644 ${HTMLMAN} ${DESTDIR}${docdir}/.
	install -m 644 ${HTMLDOC} ${DESTDIR}${docdir}/.

uninstall:
	for d in ${PROGS}; do \
	  echo rm -f ${DESTDIR}${bindir}/$$d; \
	  rm -f ${DESTDIR}${bindir}/$$d; \
	done
	for d in ${MANPAGES}; do \
	  echo rm -f ${DESTDIR}${mandir}/man1/$$d; \
	  rm -f ${DESTDIR}${mandir}/man1/$$d; \
	done
	for d in ${HTMLMAN} ${HTMLDOC{; do \
	  echo rm -f ${DESTDIR}${docdir/$$d; \
	  rm -f ${DESTDIR}$${docdir}/$$d; \
	done

dist:
	rm -rf rsbackup-${VERSION}
	mkdir -p rsbackup-${VERSION}/debian
	cp README rsbackup-${VERSION}
	cp COPYING rsbackup-${VERSION}
	cp Makefile rsbackup-${VERSION}
	cp ${PROGS} rsbackup-${VERSION}
	cp rsbackup.cron.pod rsbackup-${VERSION}
	cp rsbackup-mount.pod rsbackup-${VERSION}
	cp htmlman rsbackup-${VERSION}
	cp ${HTMLDOC} rsbackup-${VERSION}
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
	rm -f ${MANPAGES}
	rm -f ${HTMLMAN}
