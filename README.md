
# Overview

This is the installer for the [RasPlex project](http://rasplex.com)

Planned features include:


* Allow version selection (stable, testing/bleeding, developer)
* Allow upgrading to newer versions (not full image install)
* Backup creation
* Use offline files
* RPi config

# Platform Deloyment

## Windows

### Setup and Configuration

Sources:

[this post](http://falsinsoft.blogspot.ca/2011/12/building-static-qt-on-windows-with.html)
[this wiki](http://qt-project.org/wiki/How_to_build_a_static_Qt_version_for_Windows_with_gcc)



These are the steps I took to compile Qt statically. I'm sure there are other ways too.

* Download and install QtSDK (MinGW version, I used 5.0.1) 
* Download Qt sources (I used Qt 5.0.1) - i used the one bundled with qt creator
* Install a version of perl (I used strawberry perl)
* Install python
* Unpack Qt sources, e.g. in C:\Qt
* Edit C:\Qt\qt-everywhere-opensource-src-5.0.1\qtdeclarative\src\src.pro and change

```
!isEmpty(QT.gui.name) {
    SUBDIRS += \
```

to

```
qtHaveModule(gui):contains(QT_CONFIG, opengl(es1|es2)?) {
    SUBDIRS += \
``` 

(this fixes build when --no-opengl is used)

Now we have to patch some makefiles:  
OK, now we need to make few changes in  some files inside sources folder. Using Notepad.exe open the following file:

```
C:\Qt\qt-everywhere-opensource-src-x.x.x\mkspecs\win32-g++\qmake.conf

C:\Qt\qt-everywhere-opensource-src-x.x.x\qmake\Makefile.win32   
C:\Qt\qt-everywhere-opensource-src-x.x.x\qmake\Makefile.win32-g++ 
C:\Qt\qt-everywhere-opensource-src-x.x.x\qmake\Makefile.win32-g++-sh
```

Search for the following line:

```
QMAKE_LFLAGS  =
```

and change to (add -static -static-libgcc):

```
QMAKE_LFLAGS  = -static -static-libgcc
```

```
LFLAGS  = [other-params...]
```

and change to add

```
LFLAGS  = -static-libgcc [other-params...]
```

### Building QT



Open "Qt 5.0.1 for Desktop (MinGW 4.7)" from start menu. It's basically cmd.exe with some extra environment variables:

```bash
cd C:\path\to\qt
configure -static -release -opensource -nomake tools -nomake examples -no-accessibility -no-openvg -no-nis -no-iconv -no-inotify -no-fontconfig -qt-zlib -no-icu -no-gif -no-libjpeg -no-freetype -no-vcproj -strip -no-plugin-manifests -no-openssl -no-dbus -no-audio-backend -no-qml-debug -no-directwrite -no-native-gestures -no-mp -no-rtti -no-c++11 -no-opengl -system-zlib
mingw32-make
```

This will generate a new qmake in the Qt directory, which knows how to statically compile. Use this one instead of the default.

For example, mine was at "C:\Qt\Qt-5.0.2\5.0.2\Src\bin"

### Building the installer

Command line:

From the rasplex-installer source directory, uun qmake as generated above from Qt shell (Qt 5.0.1 for desktop (MinGW 4.7)), then run mingw32-make:

```
"C:\Qt\Qt-5.0.2\5.0.2\Src\bin\qmake"
mingw32-make
```

Using Qt creator:

* Open rasplex-installer project in Qt Creator
* Go to Tools->Options->Build & Run->Qt Versions and click add. Select C:\Qt\qt-everywhere-opensource-src-5.0.1\qtbase\bin\qmake.exe
* Go to Tools->Options->Build & Run->Kits and clone the existing kit. Change Qt version of that kit to the static version.
* Apply and close Options
* Make rasplex-installer the active project and go to the Projects tab
* Click "Add kit" and select the new (static) clone
* Make sure that the build configuration is set to Release
* Build and prosper!


### Packaging

The static libs make the exe quite large, so it's a good idea to compress it using [upx](http://upx.sourceforge.net/#downloadupx)

This should take it from about 12MB down to about 4MB

## OS X

### Configuration and setup

You'll need to use Qt 5.2+, at time of writing we were using the [betas](http://download.qt-project.org/development_releases/qt/5.2/5.2.0-beta1/)

Make sure to install everything (including source components).

You can't really do a static build very easily... so we just use "macdeployqt", which is located inside "Clang/bin" directory of whereever you installed Qt.

### Building the installer

Just open the project in Qt creator and build.

### Packaging

To bundle for osx, use "macdeployqt" on the target app. This should take a few seconds, and bloats the app up to about 20MB. This is because it copies in al the Qt frameworks.

Next, merge the included osx/GetRasplex.app, to get all the app goodies (script to run as admin, icon, etc). GetRasplex.app was created by just opening main.scpt in apple script editor, and exporting it as an app, then overwriting our own icon into the app. We save the skelaton for ease of deployment.

It should look like this:

```
path-to-qt/clang/bin/macdeployqt build.blahblah-Release/rasplex-installer.app
cp -r build.blahblah/rasplex-installer.app /tmp/GetRasplex.app
cp -r osx/GetRasplex.app/ /tmp/GetRasplex.app
```

Now, to shrink it down, go ahead and grab [dmgcreator](http://dmgcreator.sourceforge.net/en/), and add GetRasplex.app to it, selecting "zlib" compression.

This should take the bundle down to about 8MB from 20. Not bad.



## Linux

### Configuration and setup


You'll need to use Qt 5.2+, at time of writing we were using the [betas](http://download.qt-project.org/development_releases/qt/5.2/5.2.0-beta1/)

Make sure to install everything (including source components).

### Building Qt

Go to the source directory where you installed qt, and enter "qtbase". Run the following configure command:


```bash

./configure -static -debug-and-release -opensource -confirm-license  -nomake tools -nomake examples -no-opengl -no-accessibility -process -qt-sql-sqlite -qt-zlib -qt-libpng -qt-libjpeg -no-icu  -qt-pcre -qt-xcb  -no-egl -no-cups  -no-linuxfb   -no-dbus
make -j `nproc`
```

This command is designed to minimize external dependencies, check against doconf.linux file first, as it may be more up-to-date than this readme. When the make is done, you'll have a static Qt and a qmake you can use that knows how to handle static builds.


### Building the installer

From the rasplex-installer source dir, simply run the qmake you generated above (usually at qtbase/bin/qmake), then run make.

```bash
path-to-qt-sources/qtbase/bin/qmake
make -j `nproc`
```

### Packaging

Install [upx](http://upx.sourceforge.net/#downloadupx) from your package manager, or from their site, whatever is easier. Then just run upx on the output from the build. This should bring the installer down from about 17MB to about 6.


