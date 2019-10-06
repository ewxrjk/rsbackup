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
bin_PROGRAMS=rsbackup rsbackup-graph

AM_CXXFLAGS=$(CAIROMM_CFLAGS) $(PANGOMM_CFLAGS) $(FINAL_CXXFLAGS) -I${srcdir}/../lib

POLICIES=PrunePolicyAge.cc PrunePolicyNever.cc PrunePolicyExec.cc \
	PrunePolicyDecay.cc \
	BackupPolicyDaily.cc BackupPolicyAlways.cc BackupPolicyInterval.cc

rsbackup_SOURCES=rsbackup.cc ${POLICIES}
rsbackup_LDADD=../lib/librsbackup.a $(SQLITE3_LIBS) $(BOOST_LIBS)

rsbackup_graph_SOURCES=rsbackup-graph.cc ${POLICIES}
rsbackup_graph_LDADD=../lib/librsbackup.a $(SQLITE3_LIBS) $(BOOST_LIBS) $(CAIROMM_LIBS) $(PANGOMM_LIBS)
