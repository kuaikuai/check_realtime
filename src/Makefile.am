LDADD = ../libconfig-1.4.9/lib/.libs/libconfig++.a
AM_CXXFLAGS = -D__STDC_FORMAT_MACROS -Wall -I../libconfig-1.4.9/lib/

bin_PROGRAMS = zzhelper xhubd
zzhelper_SOURCES = monitor.cpp  configfile.c file_watcher.cpp md5.c sqlite3.c file_db.cpp myexception.cpp alerter.cpp util.cpp mailman.cpp file_log.cpp TaskTimerManager.hpp file_analysis.cpp
xhubd_SOURCES = worker.cpp
