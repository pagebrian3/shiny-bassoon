#!/bin/bash
g++ main.cc VideoIcon.cpp VBrowser.cpp DbConnector.cpp -o vbrowser -std=gnu++14  -I/usr/include -I.  -L/usr/lib/ -lsqlite3 -lboost_filesystem -lboost_system -ldl -lpthread  `pkg-config --cflags --libs MagickWand` `pkg-config --cflags --libs gtkmm-3.0` -march=native -mtune=native -O3 -g

chmod +x vbrowser
rm -rf ~/.video_proj/*
