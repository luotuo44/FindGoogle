TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    main.cpp \
    DNS.cpp \
    DNS_Machine.cpp \
    Reactor.cpp \
    SocketOps.cpp \
    ConnectPort.cpp \
    IPQueue.cpp

HEADERS += \
    conn.hpp \
    DNS.hpp \
    DNS_Machine.hpp \
    Observer.hpp \
    typedefine.hpp \
    Reactor.hpp \
    Uncopyable.hpp \
    SocketOps.hpp \
    ConnectPort.hpp \
    IPQueue.hpp \
    MutexLock.hpp

LIBS += -pthread

QMAKE_CXXFLAGS += -std=c++11

OTHER_FILES += \
    dns_query.txt
