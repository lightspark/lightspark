Lightspark
==========

![GitHub release](https://img.shields.io/github/release/lightspark/lightspark.svg)
![GitHub Last Commit](https://img.shields.io/github/last-commit/lightspark/lightspark.svg)
[![Travis Status](https://img.shields.io/travis/com/lightspark/lightspark/master.svg?label=master%20branch)](https://travis-ci.com/lightspark/lightspark)

Lightspark is an open source Flash player implementation for playing
files in SWF format. Lightspark can run as a web browser plugin or as
a standalone application.

Lightspark supports SWF files written on all versions of the
ActionScript language.

Building and Installation
-------------------------

To compile this software you need to install development packages for:
* opengl
* curl
* zlib
* libavcodec
* libavresample
* libglew
* pcre
* librtmp
* cairo
* sdl2
* sdl2_mixer
* libjpeg
* libavformat
* pango
* liblzma

If JIT compilation using llvm is enabled (this is disabled by default),
you also need the development packages for llvm (version 2.8 or >= 3.0)

If compiling the PPAPI (Chromium) plugin is enabled (on by default), keep in mind that
it will replace the Adobe Flash plugin, as only one flash plugin is allowed in Chromium.

Also install the following tools:
* cmake
* nasm
* gcc (version 4.6.0 or newer) or clang

To build the software please follow these steps.

```bash
cd lightspark
mkdir obj
cd obj
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

DEBUG MODE:
To enable debug mode change the cmake command like this:

``cmake -DCMAKE_BUILD_TYPE=Debug``

The ``CMAKE_BUILD_TYPE`` options are: Debug LeanDebug Release RelWithDebInfo Profile

Execution
---------

Using `make install`, lightspark is installed in the system wide

### Browser plugin

Firefox plugin path and Firefox should show it in the about:plugins
list and in the Tools -> Add-ons -> Plugins window.

Lightspark registers itself as the plugin for
application/x-shockwave-flash and for application/x-lightspark, so it
should be recognisable in the about:plugins page. Its description
string is ``Shockwave Flash 12.1 r<current version>``. The current
version is now "r710".

Firefox is not able to deal very well with multiple plugins for the
same MIME type. If you only see a black box where a flash app should
be try to remove any other flash plugin you have installed.

### Command line

The command line version of Lightspark can play a local SWF file.
Execution: ``lightspark file.swf``

Type `lightspark` to see all command line options.

### Keyboard shortcuts

* _Ctrl+Q_: quit (standalone player only)
* _Ctrl+F_: toggle between normal and fullscreen view
* _Ctrl+M_: mute/unmute sounds
* _Ctrl+P_: show profiling data
* _Ctrl+O_: create screenshot and save it as bmp file in temp folder
* _Ctrl+C_: copy an error to the clipboard (when Lightspark fails)

### Environment variables

* ``LIGHTSPARK_USE_GNASH``: if set to 1, lightspark will fall back to gnash for older swf files
* ``LIGHTSPARK_PLUGIN_LOGLEVEL``: sets the log level (0-4) (browser plugins only)
* ``LIGHTSPARK_PLUGIN_LOGFILE``: sets the file the log will be written to (browser plugins only)
* ``LIGHTSPARK_PLUGIN_PARAMFILE``: if set, the flash variables set by the website will be written to this file (browser plugins only)

SWF Support
-----------

Many web sites do not yet work yet because the implementation is
incomplete. But we do support a number of them. See our [site compatibility page]
for more details.

[site compatibility page]: https://github.com/lightspark/lightspark/wiki/Site-Support

Testing
--------------

An easy way to compare Lightspark to Adobe Flash Player is to use the Flash Player Projector, which is simply a standalone executable of Flash that can be quickly run directly from the command line.

The Flash Player Projector can be found [here](https://fpdownload.macromedia.com/pub/flashplayer/updaters/32/flash_player_sa_linux.x86_64.tar.gz), or if that link is broken, on [this](https://www.adobe.com/support/flashplayer/debug_downloads.html) page, under the Linux category. Extract the archive, and then you have your flashplayer executable.

This should be used when testing Lightspark, to discern what are genuine bugs with Lightspark, and what are simply bugs with the SWF file itself. If you think you have found a bug, see our [bug reporting help](https://github.com/lightspark/lightspark/wiki/Reporting-Bugs) for details.
