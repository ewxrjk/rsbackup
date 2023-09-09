# Copyright © 2011, 2012, 2014-18 Richard Kettlewell.
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

# Set workspace based on test name
mkdir -p workspaces
export WORKSPACE="${PWD}/workspaces/${0##*/}"

# 
RSBACKUP="${VALGRIND} ${PWD}/../src/rsbackup --config ${WORKSPACE}/config ${VERBOSE_OPT}"

# Set defaults
srcdir="${srcdir:-.}"
BACKUP_POLICY="${BACKUP_POLICY:-daily}"
PRUNE_POLICY="${PRUNE_POLICY:-age}"
PRUNE_AGE="${PRUNE_AGE:-prune-parameter prune-age}"
MIN_BACKUPS="${MIN_BACKUPS:-prune-parameter min-backups}"

setup() {
  echo
  echo "* ==== $0 ===="

  # Figure out what the 'real' rsync is
  case $(uname -s) in
      Darwin )
          RSYNC=/usr/bin/rsync
          ;;
      * )
          RSYNC=rsync
          ;;
  esac
  export RSYNC
  RSYNC_COMMAND=${RSYNC_COMMAND:-$RSYNC}

  rm -rf ${WORKSPACE}
  mkdir ${WORKSPACE}

  rm -f ${WORKSPACE}/config

  mkdir ${WORKSPACE}/store1
  echo device1 > ${WORKSPACE}/store1/device-id
  echo "store --no-mounted ${WORKSPACE}/store1" >> ${WORKSPACE}/config
  echo "device \"device1\"" >> ${WORKSPACE}/config

  mkdir ${WORKSPACE}/store2
  echo device2 > ${WORKSPACE}/store2/device-id
  echo "store --no-mounted ${WORKSPACE}/store2" >>${WORKSPACE}/config
  echo "device device2" >> ${WORKSPACE}/config

  echo "public true" >> ${WORKSPACE}/config

  echo "pre-device-hook ${srcdir}/hook" >> ${WORKSPACE}/config
  echo "post-device-hook ${srcdir}/hook" >> ${WORKSPACE}/config

  echo "keep-prune-logs 1d" >> ${WORKSPACE}/config
  echo "backup-policy ${BACKUP_POLICY}" >> ${WORKSPACE}/config
  [ -n "$BACKUP_INTERVAL" ] && echo "backup-parameter min-interval ${BACKUP_INTERVAL}" >> ${WORKSPACE}/config
  echo "prune-policy ${PRUNE_POLICY}" >> ${WORKSPACE}/config
  [ -n "$PRUNE_PATH" ] && echo "prune-parameter path ${PRUNE_PATH}" >> ${WORKSPACE}/config
  [ -n "$DECAY_LIMIT" ] && echo "prune-parameter decay-limit ${DECAY_LIMIT}" >> ${WORKSPACE}/config

  mkdir ${WORKSPACE}/logs
  echo "logs ${WORKSPACE}/logs" >> ${WORKSPACE}/config
  echo "lock ${WORKSPACE}/lock" >> ${WORKSPACE}/config

  # Exclude graph from report
  echo 'report "title:Backup report (${RSBACKUP_DATE})"' >> ${WORKSPACE}/config
  echo 'report + "h1:Backup report (${RSBACKUP_DATE})"' >> ${WORKSPACE}/config
  echo 'report + h2:Warnings?warnings warnings' >> ${WORKSPACE}/config
  echo 'report + h2:Summary summary' >> ${WORKSPACE}/config
  echo 'report + h2:Logfiles logs' >> ${WORKSPACE}/config
  echo 'report + "h3:Pruning logs" prune-logs' >> ${WORKSPACE}/config
  echo 'report + "p:Generated ${RSBACKUP_CTIME}"' >> ${WORKSPACE}/config

  echo "rsync-command ${RSYNC_COMMAND}" >> ${WORKSPACE}/config
  # Apple's rsync is ancient
  case $(uname -s) in
      Darwin )
	  echo 'rsync-extra-options --extended-attributes' >> ${WORKSPACE}/config
	  ;;
  esac

  echo "host host1" >> ${WORKSPACE}/config
  echo "  hostname localhost" >> ${WORKSPACE}/config
  [ "${PRUNE_AGE}" != none ] && echo "  ${PRUNE_AGE} 2d" >> ${WORKSPACE}/config
  echo "  volume volume1 ${WORKSPACE}/volume1" >> ${WORKSPACE}/config
  [ "${MIN_BACKUPS}" != none ] && echo "    ${MIN_BACKUPS} 1" >> ${WORKSPACE}/config
  echo "    pre-volume-hook ${srcdir}/hook" >> ${WORKSPACE}/config
  echo "    post-volume-hook ${srcdir}/hook" >> ${WORKSPACE}/config
  echo "    check-file file1" >> ${WORKSPACE}/config
  echo "  volume volume2 ${WORKSPACE}/volume2" >> ${WORKSPACE}/config
  [ "${MIN_BACKUPS}" != none ] && echo "    ${MIN_BACKUPS} 2" >> ${WORKSPACE}/config
  echo "  volume volume3 ${WORKSPACE}/volume3" >> ${WORKSPACE}/config
  [ "${MIN_BACKUPS}" != none ] && echo "    ${MIN_BACKUPS} 2" >> ${WORKSPACE}/config
  echo "    devices *2" >> ${WORKSPACE}/config

  mkdir ${WORKSPACE}/volume1
  echo one > ${WORKSPACE}/volume1/file1
  mkdir ${WORKSPACE}/volume1/dir1
  dd if=/dev/zero bs=4096 count=1 of=${WORKSPACE}/volume1/dir1/file2

  mkdir ${WORKSPACE}/volume2
  dd if=/dev/zero bs=1024 count=2048 of=${WORKSPACE}/volume2/file3
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
    echo "$0:${BASH_LINENO[0]}: ERROR: $1 and $2 unexpectedly differ"
    cat ${WORKSPACE}/diffs
    if ${TEST_PATCH:-false}; then
      cp "$2" "$1"
    else
      exit 1
    fi
  fi
}

exists() {
  if ! [ -e "$1" ]; then
    echo "$0:${BASH_LINENO[0]}: ERROR: $1 does not exist"
    exit 1
  fi
}

absent() {
  if [ -e "$1" ]; then
    echo "$0:${BASH_LINENO[0]}: ERROR: $1 unexpectedly exists"
    exit 1
  fi
  if [ -e "$1.incomplete" ]; then
    echo "$0:${BASH_LINENO[0]}: ERROR: $1.incomplete unexpectedly exists"
    exit 1
  fi
}

s() {
  echo "$0:${BASH_LINENO[0]}: RUN:" "$@" "#" ${RSBACKUP_TIME} >&2
  if [ "$STDOUT" != "" ]; then
    exec 3>&1
    exec >"$STDOUT"
  fi
  if [ "$STDERR" != "" ]; then
    exec 4>&2
    exec 2>"$STDERR"
  fi
  ok=true
  if RUN="${RUN}" RSBACKUP_TIME="${RSBACKUP_TIME}" "$@"; then
    ok=true
  else
    ok=false
  fi
  if [ "$STDOUT" != "" ]; then
    exec 1>&3
    cat "$STDOUT" >&1
  fi
  if [ "$STDERR" != "" ]; then
    exec 2>&4
    cat "$STDERR" >&2
  fi
  ${ok}
}

fails() {
  echo "$0:${BASH_LINENO[0]}: RUN:" "$@" "#" ${RSBACKUP_TIME} >&2
  if "$@"; then
    echo "# unexpectedly succeeded" >&2
    false
  else
    echo "# failed as expected" >&2
  fi
}

exec 3>&2
