CC = g++ #clang++
CFLAGS  = -I. -std=gnu++14 -march=native -O3
LDFLAGS = `pkg-config --cflags --libs gtkmm-3.0` -L/usr/lib/ -lboost_program_options -lboost_filesystem -lboost_system -lsqlite3 -lMagick++-7.Q16HDRI
GTKFLAGS = `pkg-config --cflags gtkmm-3.0`
MAGICKFLAGS = `pkg-config --cflags Magick++`
default: vbrowser

debug: CFLAGS =  -I. -std=gnu++14 -g
debug: vbrowser

vbrowser:  VideoUtils.o VideoIcon.o DbConnector.o VBrowser.o main.o 
	$(CC) $(CFLAGS) $(LDFLAGS) -o vbrowser VideoUtils.o VideoIcon.o DbConnector.o VBrowser.o main.o

main.o:  main.cc VBrowser.h VBrowser.cpp
	$(CC) $(CFLAGS) $(GTKFLAGS) $(MAGICKFLAGS) -c main.cc

VideoIcon.o:  VideoIcon.cpp VideoIcon.h VidFile.h
	$(CC) $(CFLAGS) $(GTKFLAGS)  -c VideoIcon.cpp

VBrowser.o:  VBrowser.cpp VBrowser.h  VidFile.h
	$(CC) $(CFLAGS) $(GTKFLAGS) $(MAGICKFLAGS) -c VBrowser.cpp

DbConnector.o:  DbConnector.cpp DbConnector.h VidFile.h 
	$(CC) $(CFLAGS) -c DbConnector.cpp

VideoUtils.o:  VideoUtils.cpp VideoUtils.h VidFile.h
	$(CC) $(CFLAGS) $(MAGICKFLAGS) -c VideoUtils.cpp

clean: 
	$(RM) vbrowser *.o *~
