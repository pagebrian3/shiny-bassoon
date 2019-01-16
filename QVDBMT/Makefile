CC = g++ #clang++
CFLAGS  = -I. -std=gnu++14 -march=native -O3 -fPIC
LDFLAGS = `pkg-config --cflags --libs Qt5Widgets` -L/usr/lib/ -lboost_program_options -lboost_filesystem -lboost_timer -lsqlite3 -lMagick++-7.Q16HDRI -lmediainfo -lzen -pthread
QTFLAGS = `pkg-config --cflags Qt5Widgets`
MAGICKFLAGS = `pkg-config --cflags Magick++`
default: qvbrowser

debug: CFLAGS =  -I. -std=gnu++14 -g -Wall -fPIC
debug: qvbrowser

qvbrowser:  VidFile.o VideoUtils.o DbConnector.o QVBrowser.o qmain.o 
	$(CC) $(CFLAGS) $(LDFLAGS)  $(QTFLAGS) -o qvbrowser VidFile.o VideoUtils.o DbConnector.o QVBrowser.o qmain.o

qmain.o:  qmain.cc
	$(CC) $(CFLAGS) $(QTFLAGS) $(MAGICKFLAGS) -c qmain.cc

QVBrowser.o:  QVBrowser.cpp QVBrowser.h
	$(CC) $(CFLAGS) $(QTFLAGS) $(MAGICKFLAGS) -c QVBrowser.cpp

DbConnector.o:  DbConnector.cpp DbConnector.h 
	$(CC) $(CFLAGS) -c DbConnector.cpp

VideoUtils.o:  VideoUtils.cpp VideoUtils.h
	$(CC) $(CFLAGS) $(MAGICKFLAGS) -c VideoUtils.cpp

VidFile.o:  VidFile.cpp VidFile.h
	$(CC) $(CFLAGS) -DUNICODE -c VidFile.cpp

clean: 
	$(RM) qvbrowser *.o *~
