# Copyright © 2011, 2012, 2014 Richard Kettlewell.
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
noinst_PROGRAMS=test-date test-io test-directory test-subprocess	\
	test-unicode test-timespec test-command test-regexp test-select
rsbackup_SOURCES=Backup.cc BulkRemove.cc Check.cc Command.cc Conf.cc	\
Date.cc DeviceAccess.cc Device.cc Directory.cc Document.cc Email.cc	\
error.cc Errors.cc FileLock.cc Host.cc HTML.cc IO.cc MakeBackup.cc	\
MakeDirectory.cc Progress.cc Prune.cc Regexp.cc Report.cc Retire.cc	\
RetireDevices.cc RetireVolumes.cc rsbackup.cc Store.cc stylesheet.cc	\
Subprocess.cc Text.cc Unicode.cc Volume.cc Command.h Conf.h Date.h	\
Defaults.h DeviceAccess.h Document.h Email.h Errors.h FileLock.h IO.h	\
Regexp.h Retire.h rsbackup.h Store.h Subprocess.h Utils.h ConfBase.cc
test_date_SOURCES=Date.cc test-date.cc
test_subprocess_SOURCES=test-subprocess.cc Subprocess.cc Errors.cc IO.cc
test_unicode_SOURCES=test-unicode.cc Unicode.cc
test_io_SOURCES=test-io.cc Subprocess.cc IO.cc Errors.cc error.cc
test_directory_SOURCES=test-directory.cc Directory.cc
test_timespec_SOURCES=test-timespec.cc
test_command_SOURCES=test-command.cc Command.cc Conf.cc Device.cc	\
	Host.cc Volume.cc Directory.cc Regexp.cc Date.cc IO.cc		\
	Subprocess.cc Progress.cc error.cc Backup.cc Store.cc		\
	Errors.cc DeviceAccess.cc ConfBase.cc
test_regexp_SOURCES=test-regexp.cc Regexp.cc
test_select_SOURCES=test-select.cc Host.cc Subprocess.cc IO.cc		\
error.cc ConfBase.cc Errors.cc Volume.cc Date.cc Conf.cc Command.cc	\
Regexp.cc Progress.cc Directory.cc Backup.cc Device.cc Store.cc		\
DeviceAccess.cc
TESTS=test-date test-io test-directory test-subprocess test-unicode	\
test-timespec test-command test-regexp test-select

stylesheet.cc: ${top_srcdir}/doc/rsbackup.css
	${top_srcdir}/scripts/txt2src stylesheet < $^ > $@
