# Copyright © 2011, 2012, 2014-2017 Richard Kettlewell.
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
noinst_PROGRAMS=test-date test-io test-directory test-subprocess	\
	test-unicode test-timespec test-command test-select \
	test-confbase test-check test-device test-host test-volume 	\
	test-progress test-database test-tolines test-globfiles \
	test-lock test-split test-parseinteger test-prunedecay \
	test-eventloop test-color test-base64 test-indent test-action

AM_CXXFLAGS=$(FINAL_CXXFLAGS) -I${srcdir}/../lib

test_date_SOURCES=test-date.cc
test_date_LDADD=../lib/librsbackup.a

test_color_SOURCES=test-color.cc
test_color_LDADD=../lib/librsbackup.a

test_subprocess_SOURCES=test-subprocess.cc
test_subprocess_LDADD=../lib/librsbackup.a

test_unicode_SOURCES=test-unicode.cc
test_unicode_LDADD=../lib/librsbackup.a

test_io_SOURCES=test-io.cc
test_io_LDADD=../lib/librsbackup.a

test_directory_SOURCES=test-directory.cc
test_directory_LDADD=../lib/librsbackup.a

test_timespec_SOURCES=test-timespec.cc
test_timespec_LDADD=../lib/librsbackup.a

test_command_SOURCES=test-command.cc
test_command_LDADD=../lib/librsbackup.a $(SQLITE3_LIBS) $(BOOST_LIBS)

test_select_SOURCES=test-select.cc
test_select_LDADD=../lib/librsbackup.a $(SQLITE3_LIBS) $(BOOST_LIBS)

test_confbase_SOURCES=test-confbase.cc
test_confbase_LDADD=../lib/librsbackup.a

test_check_SOURCES=test-check.cc
test_check_LDADD=../lib/librsbackup.a $(LIBPTHREAD) $(SQLITE3_LIBS) $(BOOST_LIBS)

test_device_SOURCES=test-device.cc
test_device_LDADD=../lib/librsbackup.a

test_host_SOURCES=test-host.cc
test_host_LDADD=../lib/librsbackup.a $(SQLITE3_LIBS) $(BOOST_LIBS)

test_volume_SOURCES=test-volume.cc
test_volume_LDADD=../lib/librsbackup.a $(SQLITE3_LIBS) $(BOOST_LIBS)

test_progress_SOURCES=test-progress.cc
test_progress_LDADD=../lib/librsbackup.a

test_database_SOURCES=test-database.cc
test_database_LDADD=../lib/librsbackup.a $(SQLITE3_LIBS) $(BOOST_LIBS)

test_tolines_SOURCES=test-tolines.cc
test_tolines_LDADD=../lib/librsbackup.a

test_globfiles_SOURCES=test-globfiles.cc
test_globfiles_LDADD=../lib/librsbackup.a

test_lock_SOURCES=test-lock.cc
test_lock_LDADD=../lib/librsbackup.a

test_split_SOURCES=test-split.cc
test_split_LDADD=../lib/librsbackup.a

test_parseinteger_SOURCES=test-parseinteger.cc
test_parseinteger_LDADD=../lib/librsbackup.a

test_prunedecay_SOURCES=test-prunedecay.cc ../progs/PrunePolicyDecay.cc
test_prunedecay_LDADD=../lib/librsbackup.a $(SQLITE3_LIBS) $(BOOST_LIBS)

test_eventloop_SOURCES=test-eventloop.cc
test_eventloop_LDADD=../lib/librsbackup.a

test_base64_SOURCES=test-base64.cc
test_base64_LDADD=../lib/librsbackup.a

test_indent_SOURCES=test-indent.cc
test_indent_LDADD=../lib/librsbackup.a

test_action_SOURCES=test-action.cc
test_action_LDADD=../lib/librsbackup.a $(SQLITE3_LIBS) $(BOOST_LIBS)

TESTS=test-date test-io test-directory test-subprocess test-unicode	\
test-timespec test-command test-select test-confbase	\
test-check test-device test-host test-volume test-progress test-database \
test-tolines test-globfiles test-lock test-split test-parseinteger 	\
test-prunedecay test-eventloop test-color test-base64 test-indent \
test-action
