#!/bin/sh

if [ ! -f ./confetti_unidata.c ]; then
    echo "Please run unicode.py first or build from the release tarball (instead of repo checkout) to skip this step."
    exit 1
fi

echo "Generating build information using autoreconf"
echo "This may take a while ..."

if hash autoreconf 2>/dev/null; then
    autoreconf --verbose --install --force
    echo "Now you are ready to run ./configure"
else
    echo "Couldn't find autoreconf, aborting"
    echo "Please install autoreconf and try again"
fi
