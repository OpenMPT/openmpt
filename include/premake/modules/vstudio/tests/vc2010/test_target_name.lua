--
-- test_target_name.lua
-- Test target names.
-- Author: Nick Clark
-- Copyright (c) 2026 Jess Perkins and the Premake project
--

    local p = premake
    local suite = test.declare("vstudio_vs2010_target_name")
    local vc2010 = p.vstudio.vc2010

--
-- Setup
--

    local wks, prj

    function suite.setup()
        p.action.set("vs2010")
        wks, prj = test.createWorkspace()
    end

    local function prepare(platform)
        local cfg = test.getconfig(prj, "Debug", platform)
        vc2010.targetName(cfg)
    end


--
-- Default target name is the project name
--

    function suite.defaultTargetName()
        prepare()
        test.capture [[
<TargetName>MyProject</TargetName>
        ]]
    end


    function suite.targetWithSuffix()
        targetsuffix "_suffix"

        prepare()
        test.capture [[
<TargetName>MyProject_suffix</TargetName>
        ]]
    end


--
-- Target name with targetsuffix with AARCH architecture
--

    function suite.targetWithAarch64FilterSuffix()
        architecture "AARCH64"

        filter { "architecture:AARCH64" } -- Fails if it is set to ARM64
            targetsuffix "_ARM64"

        prepare()
        test.capture [[
<TargetName>MyProject_ARM64</TargetName>
        ]]
    end


--
-- Target name with targetsuffix with ARM64 architecture
--

    function suite.targetWithArm64FilterSuffix()
        architecture "ARM64"

        filter { "architecture:ARM64" } -- Should match when architecture is AARCH64
            targetsuffix "_ARM64"

        prepare()
        test.capture [[
<TargetName>MyProject_ARM64</TargetName>
        ]]
    end


--
-- Target name with targetsuffix and x64 architecture
--

    function suite.targetWithX64FilterSuffix()
        architecture "x64"

        filter { "architecture:x64" }
            targetsuffix "_x64"

        prepare()
        test.capture [[
<TargetName>MyProject_x64</TargetName>
        ]]
    end


--
--  Target name with targetprefix
--

    function suite.targetWithPrefix()
        targetprefix "lib"

        prepare()
        test.capture [[
<TargetName>libMyProject</TargetName>
        ]]
    end


--
-- Target name with targetprefix and targetsuffix
--

    function suite.targetWithPrefixAndSuffix()
        targetprefix "lib"
        targetsuffix "_suffix"

        prepare()
        test.capture [[
<TargetName>libMyProject_suffix</TargetName>
        ]]
    end