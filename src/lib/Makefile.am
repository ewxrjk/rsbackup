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
noinst_LIBRARIES=librsbackup.a

TAG:=$(shell git describe --tags --dirty)
AM_CXXFLAGS=$(SQLITE3_CFLAGS) $(CAIROMM_CFLAGS) $(PANGOMM_CFLAGS) $(FINAL_CXXFLAGS) -DTAG=\"${TAG}\"

librsbackup_a_SOURCES=Backup.cc BulkRemove.cc Check.cc Command.cc	\
Conf.cc \
Date.cc DeviceAccess.cc Device.cc Directory.cc Document.cc Email.cc	\
error.cc Errors.cc FileLock.cc Host.cc HTML.cc IO.cc MakeBackup.cc	\
Progress.cc Prune.h Prune.cc PrunePolicy.h PrunePolicy.cc Report.cc \
RetireDevices.cc RetireVolumes.cc Store.cc stylesheet.cc	\
Subprocess.cc Text.cc Unicode.cc Volume.cc Command.h Conf.h Date.h	\
Defaults.h DeviceAccess.h Document.h Email.h Errors.h FileLock.h IO.h	\
rsbackup.h Store.h Subprocess.h Utils.h ConfBase.cc		\
toLines.cc globFiles.cc Database.h Database.cc Report.h			\
parseInteger.cc split.cc EventLoop.cc EventLoop.h nonblock.cc		\
Action.cc Action.h BulkRemove.h Selection.h Selection.cc Color.h 	\
Color.cc parseFloat.cc Render.h Render.cc HistoryGraph.h	\
HistoryGraph.cc ColorStrategy.cc ConfDirective.h ConfDirective.cc	\
base64.cc substitute.cc timestamp.cc debug.cc ConfBase.h Volume.h	\
Host.h Backup.h Device.h Indent.h Indent.cc \
BackupPolicy.h BackupPolicy.cc

stylesheet.cc: ${top_srcdir}/doc/rsbackup.css
	${top_srcdir}/scripts/txt2src stylesheet < $^ > $@
