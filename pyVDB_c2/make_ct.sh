#!/bin/bash
g++ -o vbrowser -std=gnu++14  -I/usr/include -I. VBrowser.cpp DbConnector.cpp main.cc -L/usr/lib/ -lboost_filesystem -lboost_system -ldl -lpthread -g -march=native -mtune=native -Ofast `pkg-config --cflags --libs MagickWand` `pkg-config --cflags --libs gtkmm-3.0`

chmod +x comp_thumbs

