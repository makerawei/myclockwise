#!/bin/bash
esptool.py --chip esp32 merge_bin -o web/firmware/myclockwise_merge_esp32.bin --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000  build/bootloader/bootloader.bin \
  0x8000  build/partition_table/partition-table.bin \
  0xe000  build/ota_data_initial.bin \
  0x10000 build/clockwise.bin
