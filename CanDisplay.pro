QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH +=  NMEA2000 \
                NMEA2000_socketCAN



SOURCES += \
    CanAdaptor/canusb.c \
    NMEA2000/N2kDeviceList.cpp \
    NMEA2000/N2kGroupFunction.cpp \
    NMEA2000/N2kGroupFunctionDefaultHandlers.cpp \
    NMEA2000/N2kMessages.cpp \
    NMEA2000/N2kMsg.cpp \
    NMEA2000/N2kStream.cpp \
    NMEA2000/N2kTimer.cpp \
    NMEA2000/NMEA2000.cpp \
    NMEA2000_socketCAN/NMEA2000_SocketCAN.cpp \
    display.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    CanAdaptor/canusb.h \
    NMEA2000/N2kCANMsg.h \
    NMEA2000/N2kDef.h \
    NMEA2000/N2kDeviceList.h \
    NMEA2000/N2kGroupFunction.h \
    NMEA2000/N2kGroupFunctionDefaultHandlers.h \
    NMEA2000/N2kMessages.h \
    NMEA2000/N2kMessagesEnumToStr.h \
    NMEA2000/N2kMsg.h \
    NMEA2000/N2kStream.h \
    NMEA2000/N2kTimer.h \
    NMEA2000/N2kTypes.h \
    NMEA2000/NMEA2000.h \
    NMEA2000/NMEA2000StdTypes.h \
    NMEA2000/NMEA2000_CAN.h \
    NMEA2000/NMEA2000_CompilerDefns.h \
    NMEA2000/RingBuffer.h \
    NMEA2000/RingBuffer.tpp \
    NMEA2000_socketCAN/NMEA2000_SocketCAN.h \
    display.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    CanDisplay_en_GB.ts
CONFIG += lrelease
CONFIG += embed_translations


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /home/lee/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
