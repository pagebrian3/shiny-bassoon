#!/bin/bash   
CPP_FLAGS="-I/usr/include -I.  -std=gnu++14 -march=native -mtune=native -O3 -g"
g++ -c main.cc $CPP_FLAGS `pkg-config --cflags gtkmm-3.0`
g++ -c VideoIcon.cpp $CPP_FLAGS  `pkg-config --cflags gtkmm-3.0`
g++ -c VBrowser.cpp $CPP_FLAGS `pkg-config --cflags gtkmm-3.0` `pkg-config --cflags MagickWand`
g++ -c DbConnector.cpp $CPP_FLAGS
g++ *.o -o vbrowser $CPP_FLAGS `pkg-config --cflags --libs gtkmm-3.0` `pkg-config --cflags --libs MagickWand` -L/usr/lib/ -lboost_filesystem -lboost_system -lsqlite3

chmod +x vbrowser
rm -rf ~/.video_proj/*
