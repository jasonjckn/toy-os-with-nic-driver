#!/bin/bash

# run_bochs.sh
# mounts the correct loopback device, runs bochs, then unmounts.

sudo /sbin/losetup /dev/loop0 floppy.img
sudo bochs -f bochsrc
#sleep 1
sudo /sbin/losetup -d /dev/loop0
