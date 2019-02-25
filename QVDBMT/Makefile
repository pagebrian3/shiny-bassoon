CC = g++ #clang++
CFLAGS  = -I. -std=gnu++14 -march=native -O3 -fPIC
QTGUIFLAGS = `pkg-config --cflags Qt5Widgets`
MAGICKFLAGS = `pkg-config --cflags Magick++`
LDFLAGS = -L/usr/lib/ `pkg-config --libs Qt5Widgets`  -lboost_filesystem -lsqlite3 -lMagick++-7.Q16HDRI -lmediainfo -lzen -lfftw3 -pthread

default: qvbrowser

debug: CFLAGS =  -I. -std=gnu++14 -g -Wall -fPIC
debug: qvbrowser

qvbrowser:   VideoUtils.o DbConnector.o MetadataDialog.o ConfigDialog.o QVBrowser.o qmain.o
	$(CC) $(CFLAGS) $(LDFLAGS)  -o qvbrowser VideoUtils.o DbConnector.o ConfigDialog.o MetadataDialog.o QVBrowser.o qmain.o 

qmain.o:  qmain.cc
	$(CC) $(CFLAGS) $(QTGUIFLAGS) -c qmain.cc 

QVBrowser.o:  QVBrowser.cpp QVBrowser.h VideoUtils.h QVBMetadata.h QVBConfig.h DbConnector.h
	$(CC) $(CFLAGS) $(QTGUIFLAGS) -c QVBrowser.cpp 

ConfigDialog.o:  ConfigDialog.cpp ConfigDialog.h QVBConfig.h
	$(CC) $(CFLAGS) $(QTGUIFLAGS) -c ConfigDialog.cpp

MetadataDialog.o:  MetadataDialog.cpp MetadataDialog.h DbConnector.h
	$(CC) $(CFLAGS) $(QTGUIFLAGS) -c MetadataDialog.cpp

DbConnector.o:  DbConnector.cpp DbConnector.h 
	$(CC) $(CFLAGS) -c DbConnector.cpp

VideoUtils.o:  VideoUtils.cpp VideoUtils.h QVBMetadata.h QVBConfig.h DbConnector.h
	$(CC) $(CFLAGS) $(MAGICKFLAGS)  -DUNICODE -c VideoUtils.cpp

clean: 
	$(RM) qvbrowser *.o *~
