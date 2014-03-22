
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

References:

+ [Qt article describing qt5 static build](http://qt-project.org/wiki/How-to-build-a-static-Qt-for-Windows-MinGW)
+ [Windows mingw static build openssl](http://stackoverflow.com/questions/9379363/how-to-build-openssl-with-mingw-in-windows)

Setup:

+ You'll need Window7 Sp1, or something that you can install powershell 3 on. Or just reverse engineer the included script.
+ Install qt5 mingw windows 32 bit precompiled to get mingw environment [windows qt](http://download.qt-project.org/official_releases/qt/5.2/5.2.1/qt-opensource-windows-x86-mingw48_opengl-5.2.1.exe)
+ Install [msys](http://www.mingw.org/wiki/MSYS) and when it asks point it at the mingw dir from qt (should be in the Tools folder and have stuff like make and gcc in it).
 + Verify that gcc works. Always use mingw32-make and not plain make. The one that comes with Qt is much better.
+ Install [activestate perl](http://www.activestate.com/activeperl/downloads) as it's required to build openssl.
+ Grab a [source tarball](https://www.openssl.org/source/openssl-1.0.1f.tar.gz) from [openssl.org](https://www.openssl.org/source/) 
+ You're now ready to build Openssl.

### Building openssl

Unpack the sources, and configure/build as such:

```
perl Configure mingw no-shared no-asm --prefix=/c/OpenSSL
make depend
make
make install
```

If asked to choose a compiler, abviously choose mingw. It'll install a fuckload of manpages for some reason. Maybe you can figure out how to make it not, but i neven tried.

There might be an undocumented step that i'm missing, if there is, please add it here.

You should end up with a lib folder containing "libcrypto.a" and "libssl.a". Woot!

### Building QT

Crack open the included "windows/windows-build-qt-static.ps1", which is a... sigh... powershell script.

Notice the configure line:


```
    cmd /c 'configure.bat -static -debug-and-release -platform win32-g++ -prefix $QtDir -qt-zlib -qt-pcre -qt-libpng -qt-libjpeg -qt-freetype -opengl desktop -qt-sql-sqlite -no-openssl -opensource -confirm-license  -make libs -nomake tools -nomake examples -nomake tests -no-accessibility -no-openvg -no-nis -no-iconv -no-inotify -no-fontconfig -qt-zlib -no-icu -no-gif -no-libjpeg -no-freetype -no-vcproj -no-plugin-manifests -no-dbus -no-audio-backend -no-qml-debug -no-directwrite -no-native-gestures -no-mp -no-rtti -no-c++11 -no-opengl -system-zlib -openssl-linked OPENSSL_LIBS="-lssl -lcrypto" -L C:\Openssl\lib -I C:\Openssl\include

```

This is the most important part. The openssl-linked will tell it to use the statically linked libs, instead of relying on dynamic. The include and lib paths much match your installation. Everything else is mostly to remove dependencies to supped up build time.

Either run this powershell script or pull out the important bits using your powerful human brain, which has the distinct advantage of not involving powershell.

This will generate a new qmake in the Qt directory, which knows how to statically compile. Use this one instead of the default.

For example, mine was at "C:\Qt\Static\blahblahblah\Src\qtbase\bin\qmake.exe"

Verify you can run qmake, and you're done!

### Building the installer

Command line:

From the rasplex-installer source directory, run qmake as generated above from Qt shell (Qt 5.2.1 for desktop (MinGW 4.7)), then run mingw32-make:

```
C:\Qt\Qt-5.0.2\5.0.2\Src\bin\qmake
C:\Qt\Static\blahblahblah\Src\qtbase\bin\qmake.exe
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

This should take it from about 12MB down to about 6MB

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


