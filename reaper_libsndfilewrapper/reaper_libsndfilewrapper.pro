TEMPLATE = lib
CONFIG -= qt
DEFINES -= UNICODE
TARGET=reaper_libsndfilewrapper
QMAKE_CFLAGS_RELEASE += 
QMAKE_CXXFLAGS_RELEASE += 
HEADERS += \
    sndfile.h \
    reaper_plugin_functions.h \
    reaper_plugin.h \
    libsndfileImport.h \
    wrapperclass.h

SOURCES += \
    main.cpp \
    wrapperclass.cpp \
    libsndfileimport.cpp
