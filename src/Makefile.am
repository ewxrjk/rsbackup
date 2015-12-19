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
noinst_LIBRARIES=librsbackup.a
bin_PROGRAMS=rsbackup rsbackup-graph
noinst_PROGRAMS=test-date test-io test-directory test-subprocess	\
	test-unicode test-timespec test-command test-select \
	test-confbase test-check test-device test-host test-volume 	\
	test-progress test-database test-tolines test-globfiles \
	test-lock test-split test-parseinteger test-prunedecay \
	test-eventloop test-color
dist_noinst_SCRIPTS=check-source

AM_CXXFLAGS=$(SQLITE3_CFLAGS) $(CAIROMM_CFLAGS) $(PANGOMM_CFLAGS)

librsbackup_a_SOURCES=Backup.cc BulkRemove.cc Check.cc Command.cc	\
Conf.cc \
Date.cc DeviceAccess.cc Device.cc Directory.cc Document.cc Email.cc	\
error.cc Errors.cc FileLock.cc Host.cc HTML.cc IO.cc MakeBackup.cc	\
Progress.cc Prune.h Prune.cc Report.cc \
RetireDevices.cc RetireVolumes.cc Store.cc stylesheet.cc	\
Subprocess.cc Text.cc Unicode.cc Volume.cc Command.h Conf.h Date.h	\
Defaults.h DeviceAccess.h Document.h Email.h Errors.h FileLock.h IO.h	\
rsbackup.h Store.h Subprocess.h Utils.h ConfBase.cc		\
toLines.cc globFiles.cc Database.h Database.cc Report.h			\
parseInteger.cc split.cc EventLoop.cc EventLoop.h nonblock.cc		\
Action.cc Action.h BulkRemove.h Selection.h Selection.cc Color.h 	\
Color.cc parseFloat.cc Render.h Render.cc HistoryGraph.h	\
HistoryGraph.cc ColorStrategy.cc ConfDirective.h ConfDirective.cc

rsbackup_SOURCES=rsbackup.cc PruneAge.cc PruneNever.cc PruneExec.cc \
	PruneDecay.cc
rsbackup_LDADD=librsbackup.a $(SQLITE3_LIBS) $(BOOST_LIBS)

rsbackup_graph_SOURCES=rsbackup-graph.cc PruneAge.cc PruneNever.cc	\
	PruneExec.cc PruneDecay.cc
rsbackup_graph_LDADD=librsbackup.a $(SQLITE3_LIBS) $(BOOST_LIBS) $(CAIROMM_LIBS) $(PANGOMM_LIBS)

test_date_SOURCES=test-date.cc
test_date_LDADD=librsbackup.a

test_color_SOURCES=test-color.cc
test_color_LDADD=librsbackup.a

test_subprocess_SOURCES=test-subprocess.cc Subprocess.cc Errors.cc IO.cc \
	EventLoop.cc nonblock.cc Action.cc

test_unicode_SOURCES=test-unicode.cc
test_unicode_LDADD=librsbackup.a

test_io_SOURCES=test-io.cc
test_io_LDADD=librsbackup.a

test_directory_SOURCES=test-directory.cc
test_directory_LDADD=librsbackup.a

test_timespec_SOURCES=test-timespec.cc

test_command_SOURCES=test-command.cc
test_command_LDADD=librsbackup.a $(SQLITE3_LIBS) $(BOOST_LIBS)

test_select_SOURCES=test-select.cc
test_select_LDADD=librsbackup.a $(SQLITE3_LIBS) $(BOOST_LIBS)

test_confbase_SOURCES=test-confbase.cc
test_confbase_LDADD=librsbackup.a

test_check_SOURCES=test-check.cc
test_check_LDADD=librsbackup.a $(LIBPTHREAD) $(SQLITE3_LIBS) $(BOOST_LIBS)

test_device_SOURCES=test-device.cc
test_device_LDADD=librsbackup.a

test_host_SOURCES=test-host.cc
test_host_LDADD=librsbackup.a $(SQLITE3_LIBS) $(BOOST_LIBS)

test_volume_SOURCES=test-volume.cc
test_volume_LDADD=librsbackup.a $(SQLITE3_LIBS) $(BOOST_LIBS)

test_progress_SOURCES=test-progress.cc
test_progress_LDADD=librsbackup.a

test_database_SOURCES=test-database.cc
test_database_LDADD=librsbackup.a $(SQLITE3_LIBS) $(BOOST_LIBS)

test_tolines_SOURCES=test-tolines.cc
test_tolines_LDADD=librsbackup.a

test_globfiles_SOURCES=test-globfiles.cc
test_globfiles_LDADD=librsbackup.a

test_lock_SOURCES=test-lock.cc
test_lock_LDADD=librsbackup.a

test_split_SOURCES=test-split.cc
test_split_LDADD=librsbackup.a

test_parseinteger_SOURCES=test-parseinteger.cc
test_parseinteger_LDADD=librsbackup.a

test_prunedecay_SOURCES=test-prunedecay.cc PruneDecay.cc
test_prunedecay_LDADD=librsbackup.a $(SQLITE3_LIBS) $(BOOST_LIBS)

test_eventloop_SOURCES=test-eventloop.cc EventLoop.cc
test_eventloop_LDADD=librsbackup.a

TESTS=test-date test-io test-directory test-subprocess test-unicode	\
test-timespec test-command test-select test-confbase	\
test-check test-device test-host test-volume test-progress test-database \
test-tolines test-globfiles test-lock test-split test-parseinteger 	\
test-prunedecay test-eventloop test-color check-source

stylesheet.cc: ${top_srcdir}/doc/rsbackup.css
	${top_srcdir}/scripts/txt2src stylesheet < $^ > $@
