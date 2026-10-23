--
-- tests/base/test_fileconfig.lua
-- Test suite for the fileconfig API.
-- Copyright (c) 2026 Jess Perkins and the Premake project
--

	local p = premake
	local suite = test.declare("fileconfig")
	local fileconfig = p.fileconfig

	local wks, prj, cfg, fcfg

	function suite.setup()
		wks = workspace("MyWorkspace")
		configurations { "Debug", "Release" }
		project("MyProject")
		language("C++")
		files { "hello.cpp", "special.cpp" }
	end

	local function prepare()
		prj = test.getproject(wks, 1)
		cfg = test.getconfig(prj, "Debug")
		local node = p.project.getsourcetree(cfg.project).children["special.cpp"]
		fcfg = p.fileconfig.getconfig(node, cfg)
	end

--
-- Check that proxy returns the project config directly if filecfg is nil.
--

	function suite.proxy_returnsCfg_whenFileCfgNil()
		prepare()
		local proxy = fileconfig.proxy(cfg, nil)
		test.istrue(rawequal(cfg, proxy))
	end


--
-- Check that scalar properties set at project level are inherited when not overridden by file.
--

	function suite.proxy_scalar_inheritedFromProject()
		cppdialect "C++17"
		vectorextensions "AVX2"
		filter "files:special.cpp"
			buildoptions { "-DFOO" }
		filter {}
		prepare()

		local proxy = fileconfig.proxy(cfg, fcfg)
		test.isequal("C++17", proxy.cppdialect)
		test.isequal("AVX2", proxy.vectorextensions)
	end


--
-- Check that scalar properties set at file level override project-level properties.
--

	function suite.proxy_scalar_overriddenByFile()
		cppdialect "C++17"
		vectorextensions "AVX2"
		filter "files:special.cpp"
			vectorextensions "AVX"
		filter {}
		prepare()

		local proxy = fileconfig.proxy(cfg, fcfg)
		test.isequal("C++17", proxy.cppdialect)
		test.isequal("AVX", proxy.vectorextensions)
	end


--
-- Check that list properties from project and file configurations are merged together.
--

	function suite.proxy_list_merged()
		defines { "PROJECT_DEF" }
		enablewarnings { "switch" }
		filter "files:special.cpp"
			defines { "FILE_DEF" }
			enablewarnings { "shadow" }
		filter {}
		prepare()

		local proxy = fileconfig.proxy(cfg, fcfg)
		test.isequal({ "PROJECT_DEF", "FILE_DEF" }, proxy.defines)
		test.isequal({ "switch", "shadow" }, proxy.enablewarnings)
	end
