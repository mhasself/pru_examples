#!/bin/bash

## Unload uio_pruss and reload it with DDR allocation.
modprobe -r uio_pruss
modprobe uio_pruss extram_pool_sz=0x100000

if cat "$SLOTS" | grep BONE-PRU ; then
    echo PRU cape is loaded.
else
    echo BB-BONE-PRU-01 > $SLOTS
fi
