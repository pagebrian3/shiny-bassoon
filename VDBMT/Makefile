 CC = g++
CFLAGS  = -I. -std=gnu++14 -march=native -O3
LDFLAGS = `pkg-config --cflags --libs gtkmm-3.0` `pkg-config --cflags --libs MagickWand` -L/usr/lib/ -lboost_program_options -lboost_filesystem -lboost_system -lsqlite3 -lopencv_core -lopencv_video -lopencv_videoio -lopencv_imgcodecs -lopencv_imgproc -lboost_timer
GTKFLAGS = `pkg-config --cflags gtkmm-3.0`
WANDFLAGS = `pkg-config --cflags MagickWand`
OPENCVFLAGS = `pkg-config --cflags opencv`

default: vbrowser

debug: CFLAGS += -g
debug: vbrowser

vbrowser:  VideoIcon.o DbConnector.o VBrowser.o main.o 
	$(CC) $(CFLAGS) $(LDFLAGS) -o vbrowser VideoIcon.o DbConnector.o VBrowser.o main.o

main.o:  main.cc VBrowser.h VBrowser.cpp
	$(CC) $(CFLAGS) $(GTKFLAGS) -c main.cc

VideoIcon.o:  VideoIcon.cpp VideoIcon.h VidFile.h
	$(CC) $(CFLAGS) $(GTKFLAGS) $(WANDFLAGS) $(OPENCVFLAGS) -c VideoIcon.cpp

VBrowser.o:  VBrowser.cpp VBrowser.h  VidFile.h
	$(CC) $(CFLAGS) $(GTKFLAGS) $(WANDFLAGS) -c VBrowser.cpp

DbConnector.o:  DbConnector.cpp DbConnector.h VidFile.h 
	$(CC) $(CFLAGS) $(GTKFLAGS) -c DbConnector.cpp

clean: 
	$(RM) vbrowser *.o *~
