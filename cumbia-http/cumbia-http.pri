
# install root
#
exists(../cumbia-qt.prf) {
    include(../cumbia-qt.prf)
}


QT += network

# + ----------------------------------------------------------------- +
#
# Customization section:
#
# Customize the following paths according to your installation:
#
#
# Here qumbia-http will be installed
# INSTALL_ROOT can be specified from the command line running qmake "INSTALL_ROOT=/my/install/path"
#
# WebAssembly: includes under $${INSTALL_ROOT}/include/cumbia-random, libs under $${INSTALL_ROOT}/lib/wasm
# cumbia and cumbia-qtcontrols includes searched under $${INSTALL_ROOT}/include/cumbia and
# $${INSTALL_ROOT}/include/cumbia-qtcontrols
#
isEmpty(INSTALL_ROOT) {
    INSTALL_ROOT = /usr/local/cumbia-libs
}
#
#
# Here qumbia-ws include files will be installed
    CUMBIA_HTTP_INCLUDES=$${INSTALL_ROOT}/include/cumbia-http
#
#
# Here qumbia-tango-controls share files will be installed
#
    CUMBIA_HTTP_SHARE=$${INSTALL_ROOT}/share/cumbia-http
##
#
# Here qumbia-tango-controls libraries will be installed
    CUMBIA_HTTP_LIBDIR=$${INSTALL_ROOT}/lib
#
#
# Here qumbia-tango-controls documentation will be installed
    CUMBIA_HTTP_DOCDIR=$${INSTALL_ROOT}/share/doc/cumbia-http
#
# The name of the library
    cumbia_http_LIB=cumbia-http
#
# Here qumbia-tango-controls libraries will be installed
    CUMBIA_HTTP_LIBDIR=$${INSTALL_ROOT}/lib
#
#
# Here qumbia-tango-controls documentation will be installed
    CUMBIA_QTCONTROLS_DOCDIR=$${INSTALL_ROOT}/share/doc/cumbia-http
#

android-g++|wasm-emscripten {
} else {
    CONFIG += link_pkgconfig
    PKGCONFIG += cumbia
    PKGCONFIG += cumbia-qtcontrols
    PKGCONFIG += cumbia-http
}

android-g++ {
    INCLUDEPATH += $${INSTALL_ROOT}/include/cumbia-qtcontrols
    OBJECTS_DIR = obj-android
}
wasm-emscripten {
    INCLUDEPATH += $${INSTALL_ROOT}/include/cumbia-qtcontrols $${INSTALL_ROOT}/include/cumbia $${CUMBIA_HTTP_INCLUDES}
    OBJECTS_DIR = obj-wasm
    QMAKE_WASM_PTHREAD_POOL_SIZE=16
}

DEFINES += CUMBIA_PRINTINFO

VERSION_HEX = 0x010400
VERSION = 1.4.0

DEFINES += CUMBIA_HTTP_VERSION_STR=\"\\\"$${VERSION}\\\"\" \
    CUMBIA_HTTP_VERSION=$${VERSION_HEX}


QMAKE_CXXFLAGS += -std=c++11 -Wall

CONFIG += c++11

MOC_DIR = moc

QMAKE_CLEAN = moc \
    objects \
    Makefile \
    *.tag

QMAKE_EXTRA_TARGETS += docs

SHAREDIR = $${INSTALL_ROOT}/share

doc.commands = doxygen \
    Doxyfile;

unix:android-g++ {
    unix:INCLUDEPATH += $${INSTALL_ROOT}/include/cumbia  $${INSTALL_ROOT}/include/cumbia-http
    unix:LIBS +=  -L/libs/armeabi-v7a/ -lcumbia
    unix:LIBS += -lcumbia-http
}

wasm-emscripten {
    CONFIG += link_prl
}

android-g++|wasm-emscripten {
    LIBS += -L$${INSTALL_ROOT}/lib/wasm -lcumbia-http -lcumbia
} else {
    OBJECTS_DIR = objects
    packagesExist(cumbia):packagesExist(cumbia-qtcontrols) {
        message("cumbia-http.pri: using pkg-config to configure cumbia cumbia-qtcontrols includes and libraries")
    } else {
        packagesExist(cumbia):packagesExist(cumbia-qtcontrols):packagesExist(cumbia-http) {
            message("cumbia-http.pri: using pkg-config to configure cumbia cumbia-qtcontrols and cumbia-http includes and libraries")
        } else {
            unix:INCLUDEPATH += /usr/local/include/cumbia  /usr/local/include/cumbia-http
            unix:LIBS +=  -L/usr/local/lib -lcumbia
            unix:LIBS += -lcumbia-http
        }
    }
}
