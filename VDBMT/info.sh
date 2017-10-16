#!/bin/bash
mediainfo --Inform="Video;%Width%,%Height%,%Duration%,%Rotation%" "$1"
