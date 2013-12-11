
Tests
=====


## Tests

libopenmpt provides some basic unit tests that check the platform for general
sanity and do some basic internal functionality testing. The test suite
requires a special libopenmpt build that includes file saving functionality
that is not included in noarmal builds.

### Running Tests

#### On Unix

Compile with

    make TEST=1 $YOURMAKEOPTIONS clean
    make TEST=1 $YOURMAKEOPTIONS check

In order to build your normal binaries again, run

    make $YOURMAKEOPTIONS clean
    make $YOURMAKEOPTIONS

A separate clean and compile cycle is necessary for the tests because
libopenmpt is built with file saving functionality when compiling the
test suite and this is not needed and not built in for normal builds.
As the build system retains no state between make invocations, you have to
provide your make options every time.

#### On Windows

Using Visual Studio 2010, compile

    libopenmpt\libopenmpt_test.sln

and run

    bin\$ARCH\libopenmpt_test.exe

from the root of the source tree.
