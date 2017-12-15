
libopenmpt built against libraries in vcpkg
===========================================

`libopenmpt.vcxproj` in this folder allows building libopenmpt against the
dependency libraries as packaged in [vcpkg](https://github.com/Microsoft/vcpkg).
It is suitable for user wide integration as activated by

    vcpkg integrate install

It depends on the following packages: `zlib mpg123 libogg libvorbis` which can
be installed by

    vcpkg install --triplet x86-windows zlib mpg123 libogg libvorbis
    vcpkg install --triplet x64-windows zlib mpg123 libogg libvorbis
    vcpkg install --triplet x86-windows-static zlib mpg123 libogg libvorbis
    vcpkg install --triplet x64-windows-static zlib mpg123 libogg libvorbis

`x86-uwp`, `x64-uwp`, `arm-uwp` and `arm64-uwp` are not supported because
`mpg123` as currently packed in vcpkg does not support these.
