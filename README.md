rsbackup
========

[![Build Status](https://api.travis-ci.com/ewxrjk/rsbackup.svg?branch=master)](https://travis-ci.com/github/ewxrjk/rsbackup)

rsbackup backs up your computer(s) to removable hard disks.  The
backup is an ordinary filesystem tree, and hard links between repeated
backups are used to save space.  Old backups are automatically pruned
after a set period of time.

Installation
------------

### Dependencies

* [rsync](http://samba.anu.edu.au/rsync/)
* [SQLite](http://www.sqlite.org/)
* [Boost](http://www.boost.org/)
* [Cairomm](https://www.cairographics.org/cairomm/) and [Pangomm](https://github.com/GNOME/pangomm) (optional)
* A C++11 compiler

### Platforms

On Debian/Ubuntu systems,
[get rsbackup.deb](http://www.greenend.org.uk/rjk/rsbackup) and
install that.

Please see [Platform Support](https://github.com/ewxrjk/rsbackup/wiki#platform-support) for platform-specific notes.

### Building

To build from source:

    autoreconf -si # only if you got it from git
    ./configure
    make check
    sudo make install

Documentation
-------------

Read [the tutorial manual](http://www.greenend.org.uk/rjk/rsbackup/rsbackup-manual.html) first.

For reference information, see the man page:

    man rsbackup

Bugs
----

Report bugs via [Github](https://github.com/ewxrjk/rsbackup/issues).

Licence
-------

Copyright © 2010-2020 Richard Kettlewell

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
