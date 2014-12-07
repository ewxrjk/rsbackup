# Copyright © 2011-2014 Richard Kettlewell.
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
TESTS=bashisms backup prune retire-device retire-volume store \
	check-file check-configs \
	check-mounted glob-store style
EXTRA_DIST=${TESTS} setup.sh hook \
	expect/10.txt expect/11.txt expect/12.txt expect/13.txt	\
	expect/14.txt expect/15.txt expect/16.txt expect/17.txt	\
	expect/18.txt expect/19.txt expect/1.txt expect/20.txt	\
	expect/21.txt expect/22.txt expect/23.txt expect/24.txt \
	expect/2.txt expect/3.txt expect/4.txt expect/5.txt	\
	expect/6.txt expect/7.txt expect/8.txt expect/9.txt	\
	expect/10.html expect/11.html expect/12.html expect/13.html	\
	expect/14.html expect/15.html expect/16.html expect/17.html	\
	expect/18.html expect/19.html expect/1.html expect/20.html	\
	expect/21.html expect/22.html expect/23.html expect/24.html \
	expect/2.html expect/3.html expect/4.html expect/5.html		\
	expect/6.html expect/7.html expect/8.html expect/9.html	\
	expect/14-stderr.txt expect/21-stderr.txt \
	expect/empty.txt \
	configs/empty/config \
	expect/include.txt \
	configs/include/config						\
	configs/include/config.d/z configs/include/config.d/backup~	\
	configs/include/config.d/empty					\
	configs/include/config.d/\#recovery\#				\
	configs/include/config.d/.dotfile configs/include/config.d/a

clean-local:
	rm -rf $(patsubst %,w-%,${TESTS})
