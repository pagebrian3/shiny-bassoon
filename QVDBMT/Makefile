CC = g++ #clang++
CFLAGS  = -I. -std=gnu++14 -march=native -O3 -fPIC
QTGUIFLAGS = `pkg-config --cflags Qt5Widgets`
QTMMFLAGS = `pkg-config --cflags Qt5Multimedia`
MAGICKFLAGS = `pkg-config --cflags Magick++`
LDFLAGS = -L/usr/lib/ `pkg-config --libs Qt5Widgets` `pkg-config --libs Qt5Multimedia` -lboost_filesystem -lboost_timer -lsqlite3 -lMagick++-7.Q16HDRI -pthread

default: qvbrowser

debug: CFLAGS =  -I. -std=gnu++14 -g -Wall -fPIC
debug: qvbrowser

qvbrowser:   VideoUtils.o DbConnector.o ConfigDialog.o QVBrowser.o qmain.o #VidFile.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(QTGUIFLAGS) $(QTMMFLAGS) -o qvbrowser VideoUtils.o DbConnector.o ConfigDialog.o QVBrowser.o qmain.o #VidFile.o 

qmain.o:  qmain.cc
	$(CC) $(CFLAGS) $(QTGUIFLAGS) -c qmain.cc 

QVBrowser.o:  QVBrowser.cpp QVBrowser.h
	$(CC) $(CFLAGS) $(QTGUIFLAGS) -c QVBrowser.cpp 

ConfigDialog.o:  ConfigDialog.cpp ConfigDialog.h
	$(CC) $(CFLAGS) $(QTGUIFLAGS) -c ConfigDialog.cpp

DbConnector.o:  DbConnector.cpp DbConnector.h 
	$(CC) $(CFLAGS) -c DbConnector.cpp

VideoUtils.o:  VideoUtils.cpp VideoUtils.h
	$(CC) $(CFLAGS) $(MAGICKFLAGS) $(QTMMFLAGS) -c VideoUtils.cpp

#VidFile.o:  VidFile.cpp VidFile.h
#	$(CC) $(CFLAGS) $(QTMMFLAGS) -DUNICODE -c VidFile.cpp

clean: 
	$(RM) qvbrowser *.o *~
