#!/bin/bash

rm -rf ~/.video_proj
rm -rf /tmp/qvdbtmp
VA_DRIVER_NAME='iHD' QT_QPA_PLATFORMTHEME=qt6ct gdb qvbrowser-deb
