CC = g++ #clang++
CFLAGS  = -I. -std=gnu++14 -march=native -O3
LDFLAGS = `pkg-config --libs gtkmm-3.0` -pthread -L/usr/lib/ -lboost_program_options -lboost_filesystem -lboost_system -lsqlite3 -lMagick++-7.Q16HDRI -lmediainfo -lzen -lboost_timer
GTKFLAGS = `pkg-config --cflags gtkmm-3.0`
MAGICKFLAGS = `pkg-config --cflags Magick++`
default: vbrowser

debug: CFLAGS =  -I. -std=gnu++14 -g -Wall
debug: vbrowser

vbrowser:  VidFile.o VideoUtils.o VideoIcon.o DbConnector.o VBrowser.o main.o 
	$(CC) $(CFLAGS) $(LDFLAGS) -o vbrowser VidFile.o VideoUtils.o VideoIcon.o DbConnector.o VBrowser.o main.o

main.o:  main.cc
	$(CC) $(CFLAGS) $(GTKFLAGS) $(MAGICKFLAGS) -c main.cc

VideoIcon.o:  VideoIcon.cpp VideoIcon.h
	$(CC) $(CFLAGS) $(GTKFLAGS) -c VideoIcon.cpp

VBrowser.o:  VBrowser.cpp VBrowser.h
	$(CC) $(CFLAGS) $(GTKFLAGS) $(MAGICKFLAGS) -c VBrowser.cpp

DbConnector.o:  DbConnector.cpp DbConnector.h 
	$(CC) $(CFLAGS) -c DbConnector.cpp

VideoUtils.o:  VideoUtils.cpp VideoUtils.h
	$(CC) $(CFLAGS) $(MAGICKFLAGS) -c VideoUtils.cpp

VidFile.o:  VidFile.cpp VidFile.h
	$(CC) $(CFLAGS) -DUNICODE -c VidFile.cpp

clean: 
	$(RM) vbrowser *.o *~
