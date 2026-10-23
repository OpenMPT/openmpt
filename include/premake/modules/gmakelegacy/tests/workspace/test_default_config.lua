--
-- tests/actions/make/test_default_config.lua
-- Validate generation of default configuration block for makefiles.
-- Copyright (c) 2012-2015 Jess Perkins and the Premake project
--

	local suite = test.declare("make_default_config")

	local p = premake


--
-- Setup/teardown
--

	local wks, prj

	function suite.setup()
		wks = test.createWorkspace()
	end

	local function prepare()
		prj = test.getproject(wks, 1)
		p.makelegacy.defaultconfig(prj)
	end


--
-- Verify the handling of the default setup: Debug and Release, no platforms.
--

	function suite.defaultsToFirstBuildCfg_onNoPlatforms()
		prepare()
		test.capture [[
ifndef config
  config=debug
endif
		]]
	end


--
-- Verify handling of build config/platform combination.
--

	function suite.defaultsToFirstPairing_onPlatforms()
		platforms { "Win32", "Win64" }
		prepare()
		test.capture [[
ifndef config
  config=debug_win32
endif
		]]
	end


--
-- If the project excludes a workspace build cfg, it should be skipped
-- over as the default config as well.
--

	function suite.usesFirstValidPairing_onExcludedConfig()
		platforms { "Win32", "Win64" }
		removeconfigurations { "Debug" }
		prepare()
		test.capture [[
ifndef config
  config=release_win32
endif
		]]
	end


--
-- Verify handling of defaultplatform
--

	function suite.defaultsToSpecifiedPlatform()
		platforms { "Win32", "Win64" }
		defaultplatform "Win64"
		prepare()
		test.capture [[
ifndef config
  config=debug_win64
endif
		]]
	end


--
-- Verify handling of defaultconfiguration
--

	function suite.defaultsToSpecifiedConfiguration()
		defaultconfiguration "Release"
		prepare()
		test.capture [[
ifndef config
  config=release
endif
		]]
	end


--
-- Verify handling of defaultconfiguration and defaultplatform together
--

	function suite.defaultsToSpecifiedConfigurationAndPlatform()
		platforms { "Win32", "Win64" }
		defaultconfiguration "Release"
		defaultplatform "Win64"
		prepare()
		test.capture [[
ifndef config
  config=release_win64
endif
		]]
	end


--
-- Verify that invalid defaultconfiguration falls back to first
--

	function suite.fallsBackToFirstConfig_onInvalidConfiguration()
		defaultconfiguration "NonExistent"
		prepare()
		test.capture [[
ifndef config
  config=debug
endif
		]]
	end


--
-- Verify that invalid defaultplatform falls back to first platform
--

	function suite.fallsBackToFirstPlatform_onInvalidPlatform()
		platforms { "Win32", "Win64" }
		defaultplatform "ARM"
		prepare()
		test.capture [[
ifndef config
  config=debug_win32
endif
		]]
	end


--
-- Verify case-insensitive matching for defaultconfiguration
--

	function suite.caseInsensitive_forConfiguration()
		defaultconfiguration "RELEASE"
		prepare()
		test.capture [[
ifndef config
  config=release
endif
		]]
	end


--
-- Verify case-insensitive matching for defaultplatform
--

	function suite.caseInsensitive_forPlatform()
		platforms { "Win32", "Win64" }
		defaultplatform "WIN32"
		prepare()
		test.capture [[
ifndef config
  config=debug_win32
endif
		]]
	end


--
-- Verify priority: valid defaultplatform with invalid defaultconfiguration
--

	function suite.prefersValidPlatform_whenConfigInvalid()
		platforms { "Win32", "Win64" }
		defaultconfiguration "NonExistent"
		defaultplatform "Win64"
		prepare()
		test.capture [[
ifndef config
  config=debug_win64
endif
		]]
	end
