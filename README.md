Lightspark
==========

![GitHub release](https://img.shields.io/github/release/lightspark/lightspark.svg)
![GitHub Last Commit](https://img.shields.io/github/last-commit/lightspark/lightspark.svg)
[![Github Actions Status](https://img.shields.io/github/workflow/status/lightspark/lightspark/ci/master)](https://github.com/lightspark/lightspark/actions)

Lightspark is an open source Flash player implementation for playing files in the SWF format. Lightspark can run as a web browser plugin or as a standalone application.

Lightspark supports SWF files written on all versions of the ActionScript language.

Building and Installation
-------------------------

In preparation for building Lightspark, you need development packages for:
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

The following tools are also required:
* cmake
* nasm
* gcc (version 4.6.0 or newer) or clang

To install these, run the following command(s):
### Ubuntu (tested on 19.10):
```
sudo apt install git gcc nasm cmake gettext libcurl4-gnutls-dev libsdl2-mixer-dev libsdl2-dev libpango1.0-dev libcairo2-dev libavcodec-dev libavresample-dev libglew-dev librtmp-dev libjpeg-dev libavformat-dev liblzma-dev
```

### Fedora (tested on 33):
RPMFusion is required and will be enabled as part of this process.

```
sudo dnf install https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm
sudo dnf builddep lightspark
```
If you want commands for a distro not listed here, please [create an issue](https://github.com/lightspark/lightspark/issues) if it doesn't already exist.

If JIT compilation using llvm is enabled (this is disabled by default), you also need the development packages for llvm (version 2.8 or >= 3.0).

If compiling the PPAPI (Chromium) plugin is enabled (on by default), keep in mind that it will replace the Adobe Flash plugin, as only one Flash plugin is allowed in Chromium.

There are two ways of building Lightspark. You can use the included script, by running `./build.sh`. or for a debug build, run `./build.sh -d`. You can also do it manually, with the following commands:

```bash
cd lightspark
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

The ``CMAKE_BUILD_TYPE`` options are: Debug LeanDebug Release RelWithDebInfo Profile

If you run into issues building Lightspark, firstly try deleting the contents of `obj`, and run `./build.sh` file again. If you continue to have issues, please [let us know](https://github.com/lightspark/lightspark/issues).

Usage
---------

### Browser plugin

Firefox plugin path and Firefox should show it in the about:plugins list and in the Tools -> Add-ons -> Plugins window.

Lightspark registers itself as the plugin for application/x-shockwave-flash and for application/x-lightspark, so it should be recognisable in the about:plugins page. Its description string is ``Shockwave Flash 12.1 r<current version>``. The current version is now "r710".

Firefox is not able to deal very well with multiple plugins for the same MIME type. If you only see a black box where a flash app should be, try to remove any other flash plugin you have installed.

### Command line

The command line version of Lightspark can play a local SWF file.

``lightspark file.swf``

Type `lightspark` to see all command line options.

### Keyboard shortcuts

* _Ctrl+Q_: quit (standalone player only)
* _Ctrl+F_: toggle between normal and fullscreen view
* _Ctrl+M_: mute/unmute sounds
* _Ctrl+P_: show profiling data
* _Ctrl+S_: create screenshot and save it as bmp file in temp folder
* _Ctrl+C_: copy an error to the clipboard (when Lightspark fails)

### Environment variables

* ``LIGHTSPARK_USE_GNASH``: if set to 1, lightspark will fall back to gnash for older swf files
* ``LIGHTSPARK_PLUGIN_LOGLEVEL``: sets the log level (0-4) (browser plugins only)
* ``LIGHTSPARK_PLUGIN_LOGFILE``: sets the file the log will be written to (browser plugins only)
* ``LIGHTSPARK_PLUGIN_PARAMFILE``: if set, the flash variables set by the website will be written to this file (browser plugins only)

SWF Support
-----------

Many applications do not yet work yet because the implementation is
incomplete. But we do support a number of them. See our [compatibility page]
for more details.

[compatibility page]: https://github.com/lightspark/lightspark/wiki/Status-of-Lightspark-support

Testing
-----------

An easy way to compare Lightspark to Adobe Flash Player is to use the Flash Player Projector, which is simply a standalone executable of Flash that can be quickly run directly from the command line.

The Flash Player Projector can be found [here](https://fpdownload.macromedia.com/pub/flashplayer/updaters/32/flash_player_sa_linux.x86_64.tar.gz), or if that link is broken, on [this](https://www.adobe.com/support/flashplayer/debug_downloads.html) page, under the Linux category. Extract the archive, and then you have your flashplayer executable.

This should be used when testing Lightspark, to discern what are genuine bugs with Lightspark, and what are simply bugs with the SWF file itself. If you think you have found a bug, see our [bug reporting help](https://github.com/lightspark/lightspark/wiki/Reporting-Bugs) for details.
