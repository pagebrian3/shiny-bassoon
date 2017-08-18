#!/bin/bash
g++ -o comp_thumbs -std=gnu++14 -I/usr/include -I/usr/include/ImageMagick-6 -I. comp_thumbs.cpp -L/usr/lib/ -lboost_filesystem -lboost_system -ldl -lpthread -lMagick++-6.Q16HDRI -march=native -mtune=native -Ofast -DMAGICKCORE_HDRI_ENABLE=0 -DMAGICKCORE_QUANTUM_DEPTH=16

chmod +x comp_thumbs

./pyBrowser.py
