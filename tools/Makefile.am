# Copyright © 2011, 2012 Richard Kettlewell.
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
EXTRA_DIST=rsbackup.config rsbackup.cron.in rsbackup.daily		\
rsbackup.defaults rsbackup.devices rsbackup.hourly rsbackup.monthly	\
rsbackup-mount.in rsbackup.weekly

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

clean-local:
	rm -f *.new

distclean-local:
	rm -f rsbackup.cron rsbackup-mount rsbackup-snapshot-hook
