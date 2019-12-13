CC = g++ #clang++
CFLAGS  = -I. -std=gnu++17 -march=native -O3 -fPIC
QTGUIFLAGS = `pkg-config --cflags Qt5Widgets`
QTMMFLAGS = `pkg-config --cflags Qt5MultimediaWidgets`
MAGICKFLAGS = `pkg-config --cflags Magick++-7.Q8`
MAGICKLD = `pkg-config --libs Magick++-7.Q8`
QTMMLD = `pkg-config --libs Qt5MultimediaWidgets`
LDFLAGS = -L/usr/lib/ -lavutil -lswscale -lavcodec -lavformat -lboost_timer -lsqlite3 $(QTMMLD) $(MAGICKLD) -lfftw3f -lzstd -pthread

default: qvbrowser mdImporter

debug: CFLAGS =  -I. -std=gnu++17 -g -Wall -fPIC
debug: qvbrowser mdImporter

buildprof: CFLAGS += -fprofile-dir=data/pgo -fprofile-generate=data/pgo
buildprof: qvbrowser mdImporter

makepgo: CFLAGS += -fprofile-dir=data/pgo -fprofile-use=data/pgo -fprofile-correction
makepgo: qvbrowser mdImporter

qvbrowser:   VideoUtils.o DbConnector.o QVBrowser.o qmain.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o qvbrowser VideoUtils.o DbConnector.o QVBrowser.o qmain.o 

qmain.o:  qmain.cc
	$(CC) $(CFLAGS) $(QTGUIFLAGS) -c qmain.cc 

QVBrowser.o:  QVBrowser.cpp QVBrowser.h ConfigDialog.h MetadataDialog.h FilterDialog.h VideoUtils.h QVBMetadata.h QVBConfig.h DbConnector.h
	$(CC) $(CFLAGS) $(QTMMFLAGS) -c QVBrowser.cpp 

DbConnector.o:  DbConnector.cpp DbConnector.h 
	$(CC) $(CFLAGS) -c DbConnector.cpp

VideoUtils.o:  VideoUtils.cpp VideoUtils.h QVBMetadata.h QVBConfig.h DbConnector.h
	$(CC) $(CFLAGS) $(MAGICKFLAGS) -c VideoUtils.cpp

mdImporter: DbConnector.o mdImporter.o
	$(CC) $(CFLAGS) -L/usr/lib/ -lsqlite3 -pthread -o mdImporter DbConnector.o mdImporter.o

mdImporter.o: mdImporter.cc QVBMetadata.h
	$(CC) $(CFLAGS) -c mdImporter.cc 

clean:
	$(RM) qvbrowser mdImporter *.o *~
