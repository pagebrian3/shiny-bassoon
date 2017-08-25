#!/bin/bash
g++ -o comp_thumbs -std=gnu++14  -I/usr/include -I. comp_thumbs.cpp -L/usr/lib/ -lboost_filesystem -lboost_system -ldl -lpthread -g -march=native -mtune=native -Ofast `pkg-config --cflags --libs MagickWand`

chmod +x comp_thumbs

