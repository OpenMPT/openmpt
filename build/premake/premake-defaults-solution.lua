
if _OPTIONS["win10"] then
	filter {}
		systemversion "10.0.16299.0"
end

	filter { "platforms:x86" }
		system "Windows"
		architecture "x32"
	filter { "platforms:x86_64" }
		system "Windows"
		architecture "x64"
	filter { "platforms:arm" }
		system "Windows"
		architecture "ARM"
if with_arm64 then
	filter { "platforms:arm64" }
		system "Windows"
		architecture "ARM64"
end
	filter {}
