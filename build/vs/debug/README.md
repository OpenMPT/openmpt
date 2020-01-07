Debug Visualizers for Visual Studio
===================================

The file `openmpt.natvis` contains Debug Visualizer rules to be used with the
Visual Studio debugger.
These visualizers will aid the debugging process when having to access several
encapsulated data types such as fixed-point numbers or on-disk integers with
defined endianness.

Note that libopenmpt puts all its code in the "OpenMPT" namespace to avoid
symbol clashes. OpenMPT itself does not currently do this (mostly to keep type
and function names clean and short in the debug view). The visualizers are
written for OpenMPT, so if you want to debug libopenmpt, you need to modify them
by prepending all internal type names with "OpenMPT::".

Basic documentation on the "natvis" visualizer format can be found e.g. at
https://docs.microsoft.com/en-us/visualstudio/debugger/create-custom-views-of-native-objects
