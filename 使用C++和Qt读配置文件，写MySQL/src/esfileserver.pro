TEMPLATE	= app
LANGUAGE	= C++
TARGET  = esfileserver

CONFIG 	+= qt warn_on 
CONFIG	+= thread debug console
DEFINES += QT3_SUPPORT

DEFINES += QT_THREAD_SUPPORT QT_THREAD_SUPPORT _CRT_SECURE_NO_WARNINGS 

win32 {
	#OBJECTS_DIR = ../../debug/esfileserver
	DESTDIR  =  ../cbin
	LIBS 	+= ../lib/icslib/objectbase.lib 	\
		       ../lib/icslib/rtdbintf.lib
}

unix{
	OBJECTS_DIR = ../debug/esfileserver
	DESTDIR  =  ../cbin
	LIBS	+=  -L../../lib/icslib -lobjectbase \
	            -L$(ICS8000_LIBDIR) -lgaia
	QMAKE_CXXFLAGS = $$QMAKE_CXXFLAGS -fpermissive
}

SOURCES	+= main.cpp  \
		./common/cdatetime.cpp \
		fileparser.cpp \
		./common/sysconfig.cpp \
		./common/staticvariable.cpp \
		../include/commgr/dllfactory.cpp \
		./common/turntomainrole.cpp
   
HEADERS +=  \  
	   ./common/writelog.h \
	   ./common/cdatetime.h \
	   fileparser.h \
	   ./common/sysconfig.h \
	   ./common/staticvariable.h \
	   ./common/defs.h \
	   ./common/turntomainrole.h
	   
	   
#The following line was inserted by qt3to4
QT += xml  sql qt3support 