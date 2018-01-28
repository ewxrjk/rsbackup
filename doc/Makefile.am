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
man_MANS=rsbackup.1 rsbackup.cron.1 rsbackup-mount.1 rsbackup-snapshot-hook.1 \
	rsbackup.5 rsbackup-graph.1
EXTRA_DIST=$(man_MANS) rsbackup-debian.html disk-encryption.html	\
rsbackup-docs.html \
rsbackup-manual.in.html rsbackup.css CHANGES.html testing.md \
partial-transfer.md \
decay.lyx decay.pdf
MANHTML=rsbackup.1.html rsbackup.cron.1.html rsbackup-mount.1.html rsbackup-snapshot-hook.1.html rsbackup.5.html rsbackup-graph.1.html

all: rsbackup-manual.html $(LYX_TARGETS)

html: $(MANHTML)

pdf: decay.pdf

rsbackup-manual.html: rsbackup-manual.in.html Makefile
	rm -f $@.new
	sed 's/_version_/${VERSION}/g' < ${srcdir}/rsbackup-manual.in.html > $@.new
	chmod 444 $@.new
	mv -f $@.new $@

%.html: %
	$(top_srcdir)/scripts/htmlman $^

# You need lyx to do make dist(check) from git, but not just make.
%.pdf: %.lyx
	if type lyx >/dev/null 2>&1; then lyx -e pdf2 $<; fi

clean-local:
	rm -f *.new

distclean-local:
	rm -f rsbackup-manual.html
	rm -f $(MANHTML)
