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

RSBACKUP="${VALGRIND} ../src/rsbackup --config config ${VERBOSE}"

setup() {
  echo
  echo "* ==== $0 ===="
  cleanup

  mkdir store1
  echo device1 > store1/device-id
  echo "store $PWD/store1" > config
  echo "device device1" >> config

  mkdir store2
  echo device2 > store2/device-id
  echo "store $PWD/store2" >> config
  echo "device device2" >> config

  echo "public" >> config

  mkdir logs
  echo "logs $PWD/logs" >> config
  echo "lock lock" >> config

  echo "host host1" >> config
  echo "  hostname localhost" >> config
  echo "  prune-age 2" >> config
  echo "  volume volume1 $PWD/volume1" >> config
  echo "    min-backups 1" >> config
  echo "  volume volume2 $PWD/volume2" >> config
  echo "    min-backups 2" >> config
  
  mkdir volume1
  echo one > volume1/file1
  mkdir volume1/dir1
  echo two > volume1/dir1/file2

  mkdir volume2
  echo three > volume2/file3
  mkdir volume2/dir2
  echo four > volume2/dir2/file4
  echo five > volume2/dir2/file5
}

cleanup() {
  rm -f config
  rm -rf store1 store2 store3
  rm -rf logs
  rm -f lock
  rm -rf volume1 volume2
  rm -f diffs
}

compare() {
  if diff -ruN "$1" "$2" > diffs; then
    :
  else
    echo "*** $1 and $2 unexpectedly differ"
    cat diffs
    exit 1
  fi
}

exists() {
  if ! [ -e "$1" ]; then
    echo "*** $1 does not exist"
    exit 1
  fi
}

absent() {
  if [ -e "$1" ]; then
    echo "*** $1 unexpectedly exists"
    exit 1
  fi
}

s() {
  echo ">" "$@" "#" ${RSBACKUP_TODAY} >&2
  "$@"
}