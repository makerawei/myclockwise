#!/bin/bash
device=`ls /dev/*cu*serial*`
if [ -z "$device" ]; then
  echo "no serial device"
  exit 1
fi
echo "using serial device: $device"
idf.py -p ${device} flash
