The `src` directory contains the source code to the command line version of
Bredbandskollen CLI, a bandwidth measurement tool.

The `old` and `older` directories contain the deprecated TPTEST-software source.

# How to build Bredbandskollen's CLI client


Note: Pre-compiled binaries for the most common platforms can be downloaded from

    http://www.bredbandskollen.se/en/bredbandskollen-cli/

On Windows, open

    src/wincli/wincli.sln

in Visual Studio 2015 or later, then choose "Build".

On all other platforms, change to the directory

    src/cli

and run the command "make" (or "gmake"). GNU Make and a compiler with support
for C++11 is required, e.g. GCC version 4.7 or later or LLVM Clang version 3.9 or later.

To use a specific compiler, do e.g.

    make CXX=clang++

To enable support for TLS/SSL, install GnuTLS version 3.5 or later, and do

    make clean
    make GNUTLS=1

To perform a bandwidth measurement using TLS, do

    ./cli --test --ssl

For more information, see "Platform Notes" below.

# How to run the CLI client

To perform a mesurement, simply run the executable program that was built using
the above steps. For more information, run it with the --help argument or read

   https://frontend.bredbandskollen.se/download/README.txt

# About the source code

The directories framework and http contain a basic C++ network programming
framework with support for "tasks" and "timers". Some of the features are
explained by demo programs in the examples directory.

The directory json11 contains a JSON library for C++ provided by Dropbox, Inc.

The directory measurement contains the bandwidth measurement engine, build atop
the framework.

The directory cli contains a command line interface to the measurement engine.

The directory qt5gui contains the source code for a GUI to the measurement
engine. To build it, Qt5 and QWebEngine are required. You must run the Qt5
version of qmake to create a Makefile before running make to build the GUI.

# Platform Notes

* Windows

The code has not been thoroughly tested on Windows. Pull requests are welcome.

Visual Studio 2015 or later is required. Visual Studio 2017 Community can be
downloaded from https://visualstudio.microsoft.com/

Open src/wincli/wincli.sln in Visual Studio, then select "Build".

* MacOS

Install Xcode from App Store, then go to src/cli and do make.
For SSL support, install Homebrew from https://brew.sh and then do

    brew install gnutls

Once GnuTLS is installed, go to src/cli and do

    make clean
    make GNUTLS=1

* Linux

Make sure g++ version 4.7 or later is installed. Then go to src/cli and do

    make

If GnuTLS version 3.5 or later is installed, including development files, do

    make clean
    make GNUTLS=1

* OpenBSD

Install gmake, llvm and gnutls using pkg_add, then go to src/cli and do

    gmake
or

    gmake GNUTLS=1

* FreeBSD

Install gmake and gnutls using pkg, then go to src/cli and do

    gmake
or

    gmake GNUTLS=1

* NetBSD

Install gmake, llvm, clang, and gnutls using pkgin, then go to src/cli and do

    gmake
or

    gmake GNUTLS=1
