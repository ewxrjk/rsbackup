# Copyright © 2014 Richard Kettlewell.
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

#
# fake_init [OPTIONS]
#
# Creates fake workspace directory.
#
# Options:
#   --autoclean       Clean up workspace on exit (default)
#   --no-autoclean    Don't clean up worksapce on exit
#
fake_init() {
    local autoclean=true
    if [ ! -z "${fake_work}" ]; then
        echo "ERROR: fake_init already called" >&2
        exit 1
    fi
    while [ $# -gt 0 ]; do
        case "$1" in
        --autoclean )
            autoclean=true
            shift
            ;;
        --no-autoclean )
            autoclean=false
            shift
            ;;
        * )
            echo "ERROR: fake_init: unknown option '$1'" >&2
            exit 1
            ;;
        esac
    done
    fake_work=`mktemp -d ${TMPDIR:-/tmp}/tmp.XXXXXXXXXX`
    PATH="${fake_work}/faked:${PATH}"
    export PATH
    if $autoclean; then
        trap fake_cleanup EXIT INT HUP TERM
    fi
}

#
# fake_reset
#
# Clears the set of faked commands.  Use this before each test.
#
fake_reset() {
    if [ -z "${fake_work}" ]; then
        echo "ERROR: fake_init not called" >&2
        exit 1
    fi
    rm -rf "${fake_work}/faked" "${fake_work}/checks"
    mkdir  "${fake_work}/faked" "${fake_work}/checks"
    fake_failed=false
}

#
# fake_cmd [OPTIONS] NAME [CMD] [--must-args ARGS]
#
# Create a faked command called NAME which executes CMD.
# The default value of CMD is 'true'.
#
# Options:
#   --must-run           This command must be run (see fake_check)
#   --must-not-run       This command must not be run (see fake_check)
#   --must-args          Require a specific argument sequence
#
fake_cmd() {
    local name cmd must_run must_not_run must_args argno limit
    must_run=false
    must_not_run=false
    must_args=false
    if [ ! -d "${fake_work}/faked" ]; then
        echo "ERROR: fake_reset not called" >&2
        exit 1
    fi
    while [ $# -gt 0 ]; do
        case "$1" in
        --must-run )
            must_run=true
            shift
            ;;
        --must-not-run )
            must_not_run=true
            shift
            ;;
        -* )
            echo "ERROR: fake_cmd: unknown option '$1'" >&2
            exit 1
            ;;
        * )
            break
            ;;
        esac
    done
    name="$1"
    shift
    if [ $# -gt 0 ] && [ "$1" != --must-args ]; then
        cmd="$1"
        shift
    else
        cmd=true
    fi
    if [ $# -gt 0 ] && [ "$1" = --must-args ]; then
        must_args=true
        shift
    else
        must_args=false
    fi
    echo "#! /usr/bin/env bash" > "${fake_work}/faked/${name}"
    echo "set -e" >> "${fake_work}/faked/${name}"
#    echo "echo \$0 \"\$@\" >&2" >> "${fake_work}/faked/${name}"
    if $must_run; then
        echo "ERROR: ${name}: was not run" > "${fake_work}/checks/${name}.run"
        echo "rm -f ${fake_work}/checks/${name}.run" \
            >> "${fake_work}/faked/${name}"
    fi
    if $must_not_run; then
        echo "echo ERROR: ${name}: was run unexpectedly >> ${fake_work}/checks/${name}.errors" \
            >> "${fake_work}/faked/${name}"
    fi
    if $must_args; then
        echo "if [ \$# != $# ]; then" >> "${fake_work}/faked/${name}"
        echo "  echo ERROR: ${name}: expected $# args got \$# >> ${fake_work}/checks/${name}.errors" \
             >> "${fake_work}/faked/${name}"
        echo "fi" >> "${fake_work}/faked/${name}"
        n=1
        limit=$#
        while [ $n -le $limit ]; do
            echo "if [ \"\$$n\" != \"$1\" ]; then" \
                >> "${fake_work}/faked/${name}"
            echo "  echo ERROR: ${name}: arg $n: expected $1 got \$$n >> ${fake_work}/checks/${name}.errors" \
                >> "${fake_work}/faked/${name}"
            echo "fi" >> "${fake_work}/faked/${name}"
            n=$(($n+1))
            shift
        done
    fi
    echo "$cmd" >> "${fake_work}/faked/${name}"
    chmod +x "${fake_work}/faked/${name}"
}

#
# fake_run [OPTIONS] [--] COMMAND ...
#
# Runs a command and checks its exit status.
# Default is to insist that it exists with status 0.
#
# Options:
#   --must-exit STATUS         must exit with a particular status
#   --must-output STRING       must write a particular string to stdout
#   --must-output-empty        must write nothing to stdout
#
#
fake_run() {
    local must_exit must_output must_output_set must_output_empty status
    must_exit=0
    must_output_set=false
    must_output_empty=false
    while [ $# -gt 0 ]; do
        case "$1" in
        --must-exit )
            shift
            must_exit="$1"
            shift
            ;;
        --must-output )
            shift
            must_output="$1"
            must_output_set=true
            shift
            ;;
        --must-output-empty )
            must_output_empty=true
            shift
            ;;
        -- )
            shift
            break
            ;;
        -* )
            echo "ERROR: fake_run: unknown option '$1'" >&2
            exit 1
            ;;
        * )
            break
            ;;
        esac
    done
    if $must_output_set || $must_output_empty; then
        if $must_output_empty; then
            touch "${fake_work}/expected"
        else
            echo "$must_output" > "${fake_work}/expected"
        fi
        set +e
        "$@" > "${fake_work}/got"
        status=$?
        set -e
    else
        set +e
        "$@"
        status=$?
        set -e
    fi
    if [ $status != $must_exit ]; then
        echo "ERROR: $1 exited with status $status (expected $must_exit)" >&2
        exit 1
    fi
    if $must_output_set || $must_output_empty; then
        if ! diff -ruN "${fake_work}/expected" "${fake_work}/got" \
            > "${fake_work}/diff"; then
            echo "ERROR: $1 gave unexpected output" >&2
            cat "${fake_work}/diff" >&2
            exit 1
        fi
    fi
}

# fake_check [OPTIONS]
#
# Reports any --must-* violations (see fake_cmd).
#
# Options:
#   --expected-fail        Don't terminate on failure
#
fake_check() {
    local fatal failed
    fatal=true
    failed=false
    if [ ! -d "${fake_work}/faked" ]; then
        echo "ERROR: fake_reset not called" >&2
        exit 1
    fi
    while [ $# -gt 0 ]; do
        case "$1" in
        --expected-fail )
            fatal=false
            shift
            ;;
        * )
            echo "ERROR: fake_check: unknown option '$1'" >&2
            exit 1
            ;;
        esac
    done
    for f in "${fake_work}/checks/"*; do
        if [ -f "${f}" ]; then
            cat "${f}" >&2
            failed=true
        fi
    done
    if $fatal && $failed; then
        exit 1
    fi
}

#
# fake_cleanup
#
# Remove workspace directory. Run by default on termination by default
# (see fake_init).
#
fake_cleanup() {
    if [ -z "${fake_work}" ]; then
        rm -rf "${fake_work}"
        unset fake_work
    fi
}
