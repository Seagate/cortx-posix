#!/bin/bash

# Re-builds re-installs KVSFS&CORTXFS RPM packages on the local machine

./scripts/build.sh make -j
./scripts/build.sh install
