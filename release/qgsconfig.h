
// QGSCONFIG.H

#ifndef QGSCONFIG_H
#define QGSCONFIG_H

// Version must be specified according to
// <int>.<int>.<int>-<any text>.
// or else upgrading old project file will not work
// reliably.
#define VERSION "2.3.0-Master"

//used in vim src/core/qgis.cpp
//The way below should work but it resolves to a number like 0110 which the compiler treats as octal I think
//because debuggin it out shows the decimal number 72 which results in incorrect version status.
//As a short term fix I (Tim) am defining the version in top level cmake. It would be good to 
//reinstate this more generic approach below at some point though
//#define VERSION_INT 230
#define VERSION_INT 20300
#define ABISYM(x) x ## 20300
//used in main.cpp and anywhere else where the release name is needed
#define RELEASE_NAME "Master"

#define QGIS_PLUGIN_SUBDIR "lib/qgis/plugins"
#define QGIS_DATA_SUBDIR "share/qgis"
#define QGIS_LIBEXEC_SUBDIR "lib/qgis"
#define QGIS_LIB_SUBDIR "lib"
#define CMAKE_INSTALL_PREFIX "/usr/local"
#define CMAKE_SOURCE_DIR "/home/aziz/Development/QGIS"

#define QSCINTILLA_VERSION_STR ""

#if defined( __APPLE__ )
//used by Mac to find system or bundle resources relative to amount of bundling
#define QGIS_MACAPP_BUNDLE 
#endif

#define QT_PLUGINS_DIR "/usr/lib/x86_64-linux-gnu/qt4/plugins"
#define OSG_PLUGINS_PATH ""

/* #undef USING_NMAKE */

#define HAVE_POSTGRESQL

#define HAVE_SPATIALITE

#define HAVE_MSSQL

/* #undef HAVE_ORACLE */

/* #undef HAVE_PYTHON */

/* #undef HAVE_TOUCH */

/* #undef HAVE_OSGEARTHQT */

/* #undef MAPSERVER_SKIP_ECW */

#endif

