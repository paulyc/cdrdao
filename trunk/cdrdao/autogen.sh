#! /bin/sh
# Run this to generate the configure script and unpack needed packages

# This deletes the (old) pccts dir and unpacks the latest version
if test -e pccts.tar.gz ; then
  rm -rf pccts
  echo "Unpacking pccts.tar.gz"
  tar xzf pccts.tar.gz
fi

# This deletes the (old) scsilib dir and unpacks the latest version
if test -e scsilib.tar.gz ; then
  rm -rf scsilib
  echo "Unpacking scsilib.tar.gz"
  tar xzf scsilib.tar.gz
fi

# Minimum version
MAJOR_VERSION=1
MINOR_VERSION=6

AMVERSION=`automake --version  | awk '/automake/ {print $4}'`

if test `echo $AMVERSION | awk -F. '{print $1}'` -lt $MAJOR_VERSION; 
then
  echo "Your version of automake is too old, you need at least automake $MAJOR_VERSION.$MINOR_VERSION"
  exit -1
fi

if test `echo $AMVERSION | awk -F. '{print $2}' | awk -F- '{print $1}'` -lt $MINOR_VERSION; then
  echo "Your version of automake is too old, you need at least automake $MAJOR_VERSION.$MINOR_VERSION"
  exit -1
fi

# Calls aclocal, automake, autoconf and al. for you
echo "Running autoreconf"
rm -fr autom4te.cache
autoreconf
