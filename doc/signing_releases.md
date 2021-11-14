
building signed updates
=======================

 *  run
    ```
    build\download_externals.cmd
    build\auto\build_openmpt_args.cmd vs2019 win10 Win32 Release 7z default
    build\auto\build_openmpt_args.cmd vs2019 win10 x64   Release 7z default
    build\auto\build_openmpt_args.cmd vs2019 win10 ARM   Release 7z default
    build\auto\build_openmpt_args.cmd vs2019 win10 ARM64 Release 7z default
    build\auto\build_openmpt_args.cmd vs2019 win7  Win32 Release 7z default
    build\auto\build_openmpt_args.cmd vs2019 win7  x64   Release 7z default
    build\auto\build_openmpt_release_packages_multiarch.cmd
    build\auto\package_openmpt_installer_multiarch_args.cmd vs2019 win10 Win32 Release 7z default
    ```
    or just `build\auto\build_openmpt_release_manual.cmd`, which does all of the
    above in one go.

 *  results are found in `bin\openmpt-pkg.win-multi.tar`

 *  The public key to verify the signatures is exported on each packaging of
    builds alongside the other build artefacts at
    `openmpt/pkg.win/${BRANCHVERSION}/OpenMPT-${VERSION}-update-publickey.jwk.json`
    .

 *  `openmpt/pkg.win/${BRANCHVERSION}/OpenMPT-${VERSION}-update.json` contains
    the update information that needs to be copied verbatim to the respective
    update channel on update.openmpt.org. This file is not signed as it itself
    is considered only informational and may be augmented with additional
    information. The files it links that contain actual code and automated
    update instructions are all signed.

 *  If the current user did not yet have a signing key on the local computer, a
    new key will be automatically generated and stored for future re-use in the
    encrypted Windows Key Store. Any such new key should be added to the set of
    allowed update signing keys in the repository at `build/signingkeys/`, as an
    individual file named appropriately to describe the key, and as a key in the
    jwkset of allowed keys in the file
    `build/signingkeys/signingkeys.jwkset.json`. The updated
    `signingkeys.jwkset.json` then needs to be copied to the https locations
    where the update check checks for the anchor keys. There is no separate key
    handling for test and release builds.
