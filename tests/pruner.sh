#! /bin/sh
# Copyright © 2015 Richard Kettlewell.
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
set -e

assert() {
  if [ "$2" != "$3" ]; then
    echo "$1: expected '$2' but got '$3'" >&2
    exit 1
  fi
}

assert PRUNE_HOST host1 "$PRUNE_HOST"
assert PRUNE_TOTAL 0 "$PRUNE_TOTAL"
assert PRUNE_ONDEVICE "3 2 1" "$PRUNE_ONDEVICE"
case "$PRUNE_VOLUME" in
volume[123] )
  ;;
* )
  echo "PRUNE_VOLUME: got '$PRUNE_VOLUME'" >&2
  exit 1
  ;;
esac
case "$PRUNE_DEVICE" in
device[12] )
  ;;
* )
  echo "PRUNE_DEVICE: got '$PRUNE_DEVICE'" >&2
  exit 1
  ;;
esac

echo 2:zap
