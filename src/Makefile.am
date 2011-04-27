bin_PROGRAMS=rsbackup
rsbackup_SOURCES=Backup.cc Command.cc Conf.cc Date.cc Device.cc		\
Document.cc Email.cc FileLock.cc Host.cc IO.cc MakeDirectory.cc		\
Prune.cc Regexp.cc Report.cc rsbackup.cc Store.cc stylesheet.cc		\
Subprocess.cc Unicode.cc Volume.cc Command.h Conf.h Date.h Defaults.h	\
Document.h Email.h Errors.h FileLock.h IO.h Regexp.h rsbackup.h		\
Store.h Subprocess.h Unicode.h


stylesheet.cc: rsbackup.css
	${top_srcdir}/scripts/txt2src stylesheet < $^ > $@

EXTRA_DIST=rsbackup.css
