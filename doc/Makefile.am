man_MANS=rsbackup.1 rsbackup.cron.1 rsbackup-mount.1
EXTRA_DIST=$(man_MANS) debian.html disk-encryption.html rsbackup-manual.html

html: rsbackup.1.html rsbackup.cron.1.html rsbackup-mount.1.html

%.1.html: %.1
	$(top_srcdir)/scripts/htmlman $^
