#!/bin/sh

SEDFILE=`mktemp`

awk '$0 !~ /^#.*/ && (NF == 2) { print "s/\\(^\\|[^A-Za-z_]\\)\\(" $1 "\\)\\($\\|[^A-Za-z0-9_]\\)/\\1" $2 "\\3/g" }' $1 > $SEDFILE 

shift
sed -f $SEDFILE -i $*
rm $SEDFILE
