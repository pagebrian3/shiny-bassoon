#!/bin/bash
g++ -o process_db.exe -g -pthread -std=gnu++14 -I. -I/usr/include -I/usr/include/c++6.2.1/ -I/usr/include/root process_db.cpp -L/usr/lib/root -lCore -lTree -lTreePlayer -lRIO -lHist -L/usr/lib -lboost_filesystem -lboost_system -lboost_thread -Ofast -march=native -mtune=native 

chmod +x process_db.exe

#./process_db.exe
