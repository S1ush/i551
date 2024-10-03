

#!/usr/bin/sh

# Must be called with dir whose contents are to be zipped.
# If dir/.zipignore exists, then it must contain zip patterns
# specifying files to be ignored.
# The zip file ${basename dir}.zip is created in the parent of dir.

if [ $# -ne 1 ] || [ ! -d $1  ]
then
    echo "$0 DIR_TO_ZIP"
    exit 1
fi

cd $1
dir=`pwd`
base=`basename $dir`
zipName=$base.zip

cd ..

rm -f $zipName

if [ -e $base/.zipignore ]
then
    zip -r $zipName $base/* -x @$base/.zipignore
else
    zip -r $zipName $base/*
fi

