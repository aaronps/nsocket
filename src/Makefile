
all: minidnsq

minidnsq: minidnsq.cpp \
	  NSocket/NSocket.cpp \
	  NSocket/NSocket.h \
	  NSocket/NSocket_facade.h \
	  NSocket/NSocketManager.cpp \
	  NSocket/NSocketManager.hpp \
	  NSocket/NTimer.h
	g++ -Wall -o minidnsq minidnsq.cpp NSocket/*.cpp

