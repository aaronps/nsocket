#
# Gererated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc.exe
CCC=g++.exe
CXX=g++.exe
FC=

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=build/Release/MinGW-Windows

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/testlisten.o \
	${OBJECTDIR}/NSocket.o \
	${OBJECTDIR}/minidnsq.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS} dist/Release/MinGW-Windows/nsocket.exe

dist/Release/MinGW-Windows/nsocket.exe: ${OBJECTFILES}
	${MKDIR} -p dist/Release/MinGW-Windows
	${LINK.cc} -o dist/Release/MinGW-Windows/nsocket ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/testlisten.o: testlisten.cpp 
	${MKDIR} -p ${OBJECTDIR}
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/testlisten.o testlisten.cpp

${OBJECTDIR}/NSocket.o: NSocket.cpp 
	${MKDIR} -p ${OBJECTDIR}
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/NSocket.o NSocket.cpp

${OBJECTDIR}/minidnsq.o: minidnsq.cpp 
	${MKDIR} -p ${OBJECTDIR}
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/minidnsq.o minidnsq.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	${RM} -r build/Release
	${RM} dist/Release/MinGW-Windows/nsocket.exe

# Subprojects
.clean-subprojects:
