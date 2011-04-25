bin_PROGRAMS=rsbackup
rsbackup_SOURCES=Command.cc Conf.cc Date.cc Device.cc Document.cc	\
Email.cc execute.cc FileLock.cc Host.cc IO.cc Prune.cc Regexp.cc	\
Report.cc rsbackup.cc Store.cc stylesheet.cc Unicode.cc Volume.cc	\
Command.h Conf.h Date.h Defaults.h Document.h Email.h Errors.h		\
FileLock.h IO.h Regexp.h rsbackup.h Store.h Unicode.h

stylesheet.cc: rsbackup.css
	${top_srcdir}/scripts/txt2src stylesheet < $^ > $@

EXTRA_DIST=rsbackup.css
