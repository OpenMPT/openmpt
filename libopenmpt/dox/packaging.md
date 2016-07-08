Packaging {#packaging}
=========


Packaging
---------


### Packaging recommendations for distribution package maintainers

 *  Use the autotools source package.
 *  Use the default set of dependencies required by the autotools package.
 *  Run the test suite in your build process.
 *  Send any build system improvement patches upstream.
 *  Do not include the libmodplug emulation layer in the default libopenmpt
    binary package. Either do not package it at all, or provide a separate
    package named libopenmpt-modplug or libmodplug-openmpt (or similar), which
    depends on libopenmpt, provides libmodplug, and conflicts with original
    libmodplug.
 *  Split openmpt123 into its own binary package because it has more
    dependencies than libopenmpt itself.
 *  Consider providing an additional openmpt123 package (in addition to the
    default openmpt123 package with all audio output drivers), built with fewer
    audio output drivers so it does not transitively depend on X11. Name that
    package and its executable openmpt123-nox (or whatever else may be common
    practice in your distribution).

