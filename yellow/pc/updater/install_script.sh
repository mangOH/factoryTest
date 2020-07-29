#!/bin/bash

UNPACK_DIR="$1"

echo "Installing mangOH Yellow system test update..."
cp "$UNPACK_DIR/yellow.wp76xx.update" /home/yellow/factoryTest/YellowTest_Shell/system/yellow_factory_test.wp76xx.update || exit 1
cp "$UNPACK_DIR/yellow.wp77xx.update" /home/yellow/factoryTest/YellowTest_Shell/system/yellow_factory_test.wp77xx.update || exit 1
cp "$UNPACK_DIR/run.sh" /home/yellow/factoryTest/YellowTest_Shell/run.sh || exit 1
cp "$UNPACK_DIR/yellow_test.sh" /home/yellow/factoryTest/YellowTest_Shell/test_scripts/yellow_test.sh || exit 1
cp "$UNPACK_DIR/mangOH-yellow-wp76xx_0.7.0-beta1-octave.spk" /home/yellow/factoryTest/YellowTest_Shell/firmware/ || exit 1
ln -sf mangOH-yellow-wp76xx_0.7.0-beta1-octave.spk /home/yellow/factoryTest/YellowTest_Shell/firmware/yellow_final_wp76xx.spk || exit 1
echo "DONE"
