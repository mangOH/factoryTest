#!/bin/bash

# This script creates a self-extracting shell script package that can be run on the test bench
# PC to install files in the appropriate locations on that PC.
#
# 1. Update the file "install_script.sh", which is the file to be run on the test bench PC
#    after the package has been extracted to the directory passed to it as its first argument.
#
# 2. Copy the appropriate source files that you will need on the test bench PC into a local
#    staging directory.
#
# 3. Run this script, giving it the path to your staging directory as its only argument.
#
# The compressed, self-extracting shell script will be output to the current working directory.
# Rename it to whatever you want and deliver it to the factory by whatever means is most
# convenient.
#
# Copyright (C) Sierra Wireless Inc.

OUTPUT_FILE=installer.sh
STAGING_DIR="$1"
TMP_DIR=$(mktemp -d /tmp/installer.XXXXXXXX) || exit 1
TARBALL=$TMP_DIR/payload.tar.bz2
SOURCE_DIR=$(dirname "$0")

echo "Copying install script into staging directory..."
cp "$SOURCE_DIR/install_script.sh" "$STAGING_DIR" &&
echo "Packing installer payload..." &&
(cd "$STAGING_DIR" && tar -cjf $TARBALL *) &&
tar tjf $TARBALL &&
echo "Generating intaller file..." &&
cat "$SOURCE_DIR/unpack.sh" "$TARBALL" > installer.sh &&
chmod a+x $OUTPUT_FILE &&
rm -r $TMP_DIR &&
echo "Installer created at \"$(realpath $OUTPUT_FILE)\""
