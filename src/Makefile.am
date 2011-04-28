bin_PROGRAMS=rsbackup
man_MANS=rsbackup.1
rsbackup_SOURCES=Backup.cc BulkRemove.cc Check.cc Command.cc Conf.cc	\
Date.cc Device.cc Directory.cc Document.cc Email.cc Errors.cc		\
FileLock.cc Host.cc HTML.cc MakeBackup.cc MakeDirectory.cc Prune.cc	\
Regexp.cc Report.cc Retire.cc RetireDevices.cc RetireVolumes.cc		\
rsbackup.cc StdioFile.cc Store.cc stylesheet.cc Subprocess.cc		\
Unicode.cc Volume.cc BulkRemove.h Check.h Command.h Conf.h Date.h	\
Defaults.h Document.h Email.h Errors.h FileLock.h IO.h Regexp.h		\
rsbackup.h Store.h Subprocess.h Unicode.h

stylesheet.cc: rsbackup.css
	${top_srcdir}/scripts/txt2src stylesheet < $^ > $@

EXTRA_DIST=rsbackup.css ${man_MANS}
