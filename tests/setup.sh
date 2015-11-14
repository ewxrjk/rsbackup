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

export WORKSPACE="${PWD}/w-${0##*/}"
RSBACKUP="${VALGRIND} ${PWD}/../src/rsbackup --config ${WORKSPACE}/config ${VERBOSE}"

PRUNE_POLICY="${PRUNE_POLICY:-age}"
PRUNE_AGE="${PRUNE_AGE:-prune-age}"
MIN_BACKUPS="${MIN_BACKUPS:-min-backups}"

setup() {
  echo
  echo "* ==== $0 ===="

  rm -rf ${WORKSPACE}
  mkdir ${WORKSPACE}

  mkdir ${WORKSPACE}/store1
  echo device1 > ${WORKSPACE}/store1/device-id
  echo "store ${WORKSPACE}/store1" > ${WORKSPACE}/config
  echo "device \"device1\"" >> ${WORKSPACE}/config

  mkdir ${WORKSPACE}/store2
  echo device2 > ${WORKSPACE}/store2/device-id
  echo "store ${WORKSPACE}/store2" >>${WORKSPACE}/config
  echo "device device2" >> ${WORKSPACE}/config

  echo "public true" >> ${WORKSPACE}/config

  echo "pre-access-hook ${srcdir:-.}/hook" >> ${WORKSPACE}/config
  echo "post-access-hook ${srcdir:-.}/hook" >> ${WORKSPACE}/config

  echo "keep-prune-logs 1" >> ${WORKSPACE}/config
  echo "prune-policy ${PRUNE_POLICY}" >> ${WORKSPACE}/config
  [ -n "$PRUNE_PATH" ] && echo "prune-parameter path ${PRUNE_PATH}" >> ${WORKSPACE}/config
  [ -n "$DECAY_LIMIT" ] && echo "prune-parameter decay-limit ${DECAY_LIMIT}" >> ${WORKSPACE}/config

  mkdir ${WORKSPACE}/logs
  echo "logs ${WORKSPACE}/logs" >> ${WORKSPACE}/config
  echo "lock ${WORKSPACE}/lock" >> ${WORKSPACE}/config

  echo "host host1" >> ${WORKSPACE}/config
  echo "  hostname localhost" >> ${WORKSPACE}/config
  [ "${PRUNE_AGE}" != none ] && echo "  ${PRUNE_AGE} 2" >> ${WORKSPACE}/config
  echo "  volume volume1 ${WORKSPACE}/volume1" >> ${WORKSPACE}/config
  [ "${MIN_BACKUPS}" != none ] && echo "    ${MIN_BACKUPS} 1" >> ${WORKSPACE}/config
  echo "    pre-backup-hook ${srcdir:-.}/hook" >> ${WORKSPACE}/config
  echo "    post-backup-hook ${srcdir:-.}/hook" >> ${WORKSPACE}/config
  echo "    check-file file1" >> ${WORKSPACE}/config
  echo "  volume volume2 ${WORKSPACE}/volume2" >> ${WORKSPACE}/config
  [ "${MIN_BACKUPS}" != none ] && echo "    ${MIN_BACKUPS} 2" >> ${WORKSPACE}/config
  echo "  volume volume3 ${WORKSPACE}/volume3" >> ${WORKSPACE}/config
  [ "${MIN_BACKUPS}" != none ] && echo "    ${MIN_BACKUPS} 2" >> ${WORKSPACE}/config
  echo "    devices *2" >> ${WORKSPACE}/config

  mkdir ${WORKSPACE}/volume1
  echo one > ${WORKSPACE}/volume1/file1
  mkdir ${WORKSPACE}/volume1/dir1
  echo two > ${WORKSPACE}/volume1/dir1/file2

  mkdir ${WORKSPACE}/volume2
  echo three > ${WORKSPACE}/volume2/file3
  mkdir ${WORKSPACE}/volume2/dir2
  echo four > ${WORKSPACE}/volume2/dir2/file4
  echo five > ${WORKSPACE}/volume2/dir2/file5

  mkdir ${WORKSPACE}/volume3
  echo six > ${WORKSPACE}/volume3/file6

  mkdir -p ${WORKSPACE}/got
}

cleanup() {
  rm -rf "${WORKSPACE}"
}

compare() {
  if diff -ruN "$1" "$2" > ${WORKSPACE}/diffs; then
    :
  else
    echo "*** $1 and $2 unexpectedly differ"
    cat ${WORKSPACE}/diffs
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
  if [ -e "$1.incomplete" ]; then
    echo "*** $1.incomplete unexpectedly exists"
    exit 1
  fi
}

s() {
  echo ">" "$@" "#" ${RSBACKUP_TODAY} >&2
  if [ -z "$STDERR" ]; then
    if [ -z "${RSBACKUP_TODAY}" ]; then
      RUN="${RUN}" "$@"
    else
      RUN="${RUN}" RSBACKUP_TODAY="${RSBACKUP_TODAY}" "$@"
    fi
  else
    if "$@" 2> "$STDERR"; then
      cat "$STDERR" >&2
    else
      cat "$STDERR" >&2
      false
    fi
  fi
}

exec 3>&2
