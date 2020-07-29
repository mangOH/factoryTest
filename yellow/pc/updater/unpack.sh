#!/bin/bash

if ! FIRST_LINE_OF_PAYLOAD=$(awk '/^LAST_LINE/ {print NR + 1; exit 0; }' < "$0") || ! [ "$FIRST_LINE_OF_PAYLOAD" ]
then
    echo "Unable to compute payload start address in \"$0\"." >&2
    exit 1
fi

echo "Creating temporary directory..."
TMP_DIR=`mktemp -d /tmp/yellowFactoryTestInstaller.XXXXXX` || { echo "Failed to create temporary directory." >&2; exit 1; }

echo "Unpacking into temporary directory..."
{ tail -n +$FIRST_LINE_OF_PAYLOAD < "$0" | tar xjv -C "$TMP_DIR"; } || { echo "Failed to unpack payload." >&2; exit 1; }

INSTALL_SCRIPT="$TMP_DIR/install_script.sh" &&
chmod u+x "$INSTALL_SCRIPT" || { echo "Failed to make install script executable." >&2; exit 1; }
"$INSTALL_SCRIPT" "$TMP_DIR" || { echo "Install script failed." >&2; exit 1; }

echo "Cleaning up..."

rm -r "$TMP_DIR" || echo "Warning: Failed to remove temporary directory \"$TMP_DIR\"." >&2

echo "DONE"

exit 0

LAST_LINE
