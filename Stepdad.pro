QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
lessThan(QT_MAJOR_VERSION, 6): QT += gamepad

CONFIG += c++11 static

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

win32: LIBS += -lws2_32
unix: LIBS += -lpthread

SOURCES += \
  framebuffer.cpp \
  libh8300h/device.c \
  libh8300h/devices/accelerometer.c \
  libh8300h/devices/battery.c \
  libh8300h/devices/bma150.c \
  libh8300h/devices/buzzer.c \
  libh8300h/devices/buttons.c \
  libh8300h/devices/eeprom.c \
  libh8300h/devices/factory_control.c \
  libh8300h/devices/generic.c \
  libh8300h/devices/generic_adc.c \
  libh8300h/devices/lcd.c \
  libh8300h/devices/led.c \
  libh8300h/dma.c \
  libh8300h/emu.c \
  libh8300h/frontend.c \
  libh8300h/interrupts.c \
  libh8300h/ir.c \
  libh8300h/logger.c \
  libh8300h/power.c \
  libh8300h/rtc.c \
  libh8300h/sci3.c \
  libh8300h/comparator.c \
  libh8300h/timer_b1.c \
  libh8300h/timer_w.c \
  main.cpp \
  mainwindow.cpp

HEADERS += \
  framebuffer.h \
  libh8300h/config.h \
  libh8300h/device.h \
  libh8300h/devices/accelerometer.h \
  libh8300h/devices/battery.h \
  libh8300h/devices/bma150.h \
  libh8300h/devices/buzzer.h \
  libh8300h/devices/buttons.h \
  libh8300h/devices/eeprom.h \
  libh8300h/devices/factory_control.h \
  libh8300h/devices/generic.h \
  libh8300h/devices/generic_adc.h \
  libh8300h/devices/lcd.h \
  libh8300h/devices/led.h \
  libh8300h/dma.h \
  libh8300h/frontend.h \
  libh8300h/interrupts.h \
  libh8300h/ir.h \
  libh8300h/logger.h \
  libh8300h/power.h \
  libh8300h/registers.h \
  libh8300h/rtc.h \
  libh8300h/sci3.h \
  libh8300h/system.h \
  libh8300h/comparator.h \
  libh8300h/timer_b1.h \
  libh8300h/timer_w.h \
  libh8300h/types.h \
  mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
  resources.qrc
