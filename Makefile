prefix=/usr/local
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
mandir=${prefix}/man

all: rsbackup.1
	perl -wc rsbackup

rsbackup.1: rsbackup
	pod2man rsbackup > rsbackup.1

installdirs:
	mkdir -p ${mandir}/man1

install: installdirs
	install -m 555 rsbackup ${bindir}/rsbackup
	install -m 444 rsbackup.1 ${mandir}/man1/rsbackup.1

uninstall:
	rm -f ${bindir}/rsbackup
	rm -f ${mandir}/man1/rsbackup.1
