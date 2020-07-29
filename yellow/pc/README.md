To Create a Factory Test PC
===========================

1. Install Linux Mint or other Ubuntu-based distro.
2. Copy (recursively) the mangOH factoryTest repository's ```yellow/pc``` directory to ```~/factoryTest/YellowTest_Shell```
3. Copy the contents of ```yellow/pc/Desktop/*``` to ```~/Desktop/```.
4. Copy the test ```.update``` file (built from ```yellow/legatoSystem```) to ```~/factoryTest/YellowTest_Shell/system/yellow_factory_test.$TARGET_TYPE.update```
5. Copy the factory ```.spk``` file to be programmed onto all the units to ```~/factoryTest/YellowTest_Shell/firmware/yellow_final_$TARGET_TYPE.spk```

NOTE: This can be done on a USB drive, such that any PC can be booted from this USB drive.
      Furthermore, this USB drive can be copied to an image file on a Linux PC and then onto other
      USB drives to create working clones of your factory test PC.  To facilitate this, it is
      recommended that you use the following partition sizes when installing Linux:
        - 20000MB - ext4 - mounted at ```/```
        - 4096MB - swap
        - 438MB - EFI
	  Larger partitions will take longer to clone and may limit your choice of which USB thumb
	  drives you can use.

Building an Installer Package
=============================

To build an executable installer file to update a factory test bench PC, see the instructions
in the comment at the top of `yellow/pc/updater/create_updater.sh`.
