#!/bin/bash
g++ main.cc VideoIcon.cpp VBrowser.cpp DbConnector.cpp -o vbrowser -std=gnu++14  -I/usr/include -I.  -L/usr/lib/ -lsqlite3 -lboost_filesystem -lboost_system -ldl -lpthread -g -O0  `pkg-config --cflags --libs MagickWand` `pkg-config --cflags --libs gtkmm-3.0` `pkg-config --cflags --libs libmediainfo`
#-march=native -mtune=native -Ofast  -DUNICODE

chmod +x vbrowser
