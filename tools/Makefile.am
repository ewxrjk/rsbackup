# Copyright © 2011, 2012, 2014, 2015 Richard Kettlewell.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
bin_SCRIPTS=rsbackup.cron rsbackup-mount rsbackup-snapshot-hook
dist_noinst_SCRIPTS=srccheck
EXTRA_DIST=rsbackup.config rsbackup.cron.in		\
rsbackup.defaults rsbackup.devices	\
rsbackup-mount.in rsbackup-snapshot-hook.in		\
${TESTS}
CRONJOBS=rsbackup.daily rsbackup.hourly rsbackup.weekly rsbackup.monthly
noinst_SCRIPTS=${CRONJOBS}
TESTS=t-bashisms t-hook-ok t-hook-fsck-ok t-hook-fsck-fail t-hook-post t-hook-nosnap \
	t-hook-dryrun

rsbackup.cron: rsbackup.cron.in Makefile
	rm -f $@.new
	sed 's/_version_/${VERSION}/g' < ${srcdir}/rsbackup.cron.in > $@.new
	chmod 555 $@.new
	mv -f $@.new $@

rsbackup-mount: rsbackup-mount.in Makefile
	rm -f $@.new
	sed 's/_version_/${VERSION}/g' < ${srcdir}/rsbackup-mount.in > $@.new
	chmod 555 $@.new
	mv -f $@.new $@

rsbackup-snapshot-hook: rsbackup-snapshot-hook.in Makefile
	rm -f $@.new
	sed 's/_version_/${VERSION}/g' < ${srcdir}/rsbackup-snapshot-hook.in > $@.new
	chmod 555 $@.new
	mv -f $@.new $@

${CRONJOBS}: Makefile
	rm -f $@.new
	echo '#! /bin/sh' > $@.new
	echo 'set -e' >> $@.new
	echo 'test -x ${bindir}/rsbackup.cron && exec ${bindir}/rsbackup.cron $(subst rsbackup.,,$@)' >> $@.new
	chmod 555 $@.new
	mv -f $@.new $@

clean-local:
	rm -f *.new

distclean-local:
	rm -f rsbackup.cron rsbackup-mount rsbackup-snapshot-hook
	rm -f rsbackup.hourly rsbackup.daily rsbackup.weekly rsbackup.monthly
