Debug Visualizers for Visual Studio
===================================

The files `autoexp.dat` and `openmpt.natvis` contain Debug Visualizer rules to
be used with the Visual Studio debugger.
These visualizers will aid the debugging process when having to access several
encapsulated data types such as fixed-point numbers or on-disk integers with
defined endianness.

Note that libopenmpt puts all its code in the "OpenMPT" namespace to avoid
symbol clashes. OpenMPT itself does not currently do this (mostly to keep type
and function names clean and short in the debug view). The visualizers are
written for OpenMPT, so if you want to debug libopenmpt, you need to modify them
by prepending all internal type names with "OpenMPT::".


Visual Studio 2008 - 2010
-------------------------

Insert the contents of `autoexp.dat` at the end of the `[Visualizer]` section in
`%VSINSTALLDIR%\Common7\Packages\Debugger\autoexp.dat`. You may need to copy the
file to a location where you have write permissions first.

Basic documentation on the "autoexp" visualizer format can be found e.g. at
http://www.virtualdub.org/blog/pivot/entry.php?id=120 


Visual Studio 2012 - 2013
-------------------------

Put `openmpt.natvis` into the following folder:
Visual Studio 2012: `%USERPROFILE%\My Documents\Visual Studio 2012\Visualizers\`
Visual Studio 2013: `%USERPROFILE%\My Documents\Visual Studio 2013\Visualizers\`


Visual Studio 2015
------------------

`openmpt.natvis` is already part of the OpenMPT solution, so it should work out
of the box.

Basic documentation on the "natvis" visualizer format can be found e.g. at
https://msdn.microsoft.com/en-US/library/jj620914(v=vs.110).aspx