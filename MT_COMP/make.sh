#!/bin/bash
g++ -o load_db -std=gnu++14 -I/usr/include -I/usr/include/root -I. load_db.cpp -L/usr/lib/ -lboost_filesystem -lboost_system -ldl -lpthread -lopencv_core -lopencv_imgproc -lopencv_videoio -L/usr/lib/root -lCore -lTree -lRIO -march=native -mtune=native -Ofast

chmod +x load_db
#rm -rf mytestfile.root

export VDPAU_DRIVER=nvidia
./load_db /run/media/ungermax/Elements/STUFF/webcam
