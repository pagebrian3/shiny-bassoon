#!/bin/bash
mediainfo --Inform="Video;%Duration%,%Rotation%" "$1"
