all: rsbackup.1
	perl -wc rsbackup

rsbackup.1: rsbackup
	pod2man rsbackup > rsbackup.1

