# Copyright © 2011 Richard Kettlewell.
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
bin_PROGRAMS=rsbackup
rsbackup_SOURCES=Backup.cc BulkRemove.cc Check.cc Command.cc Conf.cc	\
Date.cc Device.cc Directory.cc Document.cc Email.cc Errors.cc		\
FileLock.cc Host.cc HTML.cc IO.cc MakeBackup.cc MakeDirectory.cc	\
Prune.cc Regexp.cc Report.cc Retire.cc RetireDevices.cc			\
RetireVolumes.cc rsbackup.cc Store.cc stylesheet.cc Subprocess.cc	\
Unicode.cc Volume.cc Command.h Conf.h Date.h Defaults.h Document.h	\
Email.h Errors.h FileLock.h IO.h Regexp.h Retire.h rsbackup.h Store.h	\
Subprocess.h Utils.h

stylesheet.cc: rsbackup.css
	${top_srcdir}/scripts/txt2src stylesheet < $^ > $@

EXTRA_DIST=rsbackup.css
