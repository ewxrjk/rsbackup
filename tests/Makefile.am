# Copyright © 2011-2015,17-20 Richard Kettlewell.
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
TESTS=bashisms backup prune pruneage prunenever pruneexec prunedecay \
	retire-device retire-volume retire-forget store \
	check-file check-configs check-bad-configs \
	check-mounted glob-store style upgrade issue37 partial issue43 \
	issue55 issue70 issue71 prune-timeout \
	concurrency hostgroup backupdaily backupalways backupinterval
EXTRA_DIST=${TESTS} setup.sh pruner.sh hook rsync-wrap \
	expect/retire-device/create.txt \
	expect/retire-device/device2-db.txt \
	expect/retire-device/created-db.txt \
	expect/retire-device/device2.txt \
	expect/retire-device/dryrun.txt \
	expect/include.txt \
	expect/retire-volume/volume2.html \
	expect/retire-volume/dryrun.html \
	expect/retire-volume/create.txt \
	expect/retire-volume/volume2.txt \
	expect/retire-volume/created-db.txt \
	expect/retire-volume/dryrun.txt \
	expect/retire-volume/all.txt \
	expect/retire-volume/all.html \
	expect/retire-volume/all-db.txt \
	expect/retire-volume/volume2-db.txt \
	expect/retire-volume/create.html \
	expect/store/duplicate-stderr.txt \
	expect/store/duplicate.html \
	expect/store/overridden.txt \
	expect/store/overridden.html \
	expect/store/duplicate.txt \
	expect/store/overridden-stderr.txt \
	expect/store/notmounted.txt \
	expect/prune/createsecond.txt \
	expect/prune/null.txt \
	expect/prune/dryrun-db.txt \
	expect/prune/volume2.txt \
	expect/prune/volume1.txt \
	expect/prune/third.txt \
	expect/prune/dryrun.txt \
	expect/prune/null-db.txt \
	expect/prune/unselected-db.txt \
	expect/prune/created.txt \
	expect/prune/volume2-db.txt \
	expect/prune/volume1-db.txt \
	expect/prune/everything-db.txt \
	expect/prune/later-db.txt \
	expect/prune/unselected.txt \
	expect/prunedecay/prunedecay-db.txt \
	expect/pruneexec/pruneexec-db.txt \
	expect/prunenever/neverprune-db.txt \
    expect/prunetimeout/prunetimeout-db.txt \
	expect/check-file/missing.html \
	expect/check-file/missing.txt \
	expect/style/styled.txt \
	expect/style/styled.html \
	expect/empty.txt \
	expect/glob-store/everything.txt \
	expect/glob-store/everything.html \
	expect/backup/onehost.html \
	expect/backup/dryrun.html \
	expect/backup/everything.txt \
	expect/backup/onevolume.html \
	expect/backup/onevolume.txt \
	expect/backup/dryrun.txt \
	expect/backup/everything.html \
	expect/backup/onehost.txt \
	expect/outdent.txt \
	expect/pruneparam.txt \
	expect/issue43/failhookerr.txt expect/issue43/failhookout.txt expect/issue43/tempfailhook.txt \
	configs/pruneparam/config \
	configs/empty/config \
	configs/include/config						\
	configs/include/config.d/z configs/include/config.d/backup~	\
	configs/include/config.d/empty					\
	configs/include/config.d/\#recovery\#				\
	configs/include/config.d/.dotfile configs/include/config.d/a	\
	configs/outdent/config 	\
	bad-configs/badquotes.config bad-configs/badquotes.errors	\
	bad-configs/inconsistent-indent.config				\
	bad-configs/inconsistent-indent.errors				\
	bad-configs/inconsistent-outdent.config				\
	bad-configs/inconsistent-outdent.errors				\
	bad-configs/inconsistent-volume.config				\
	bad-configs/inconsistent-volume.errors				\
	bad-configs/indent-global.config				\
	bad-configs/indent-global.errors				\
	bad-configs/unrecognized.config					\
	bad-configs/unrecognized.errors

clean-local:
	rm -rf $(patsubst %,w-%,${TESTS})
