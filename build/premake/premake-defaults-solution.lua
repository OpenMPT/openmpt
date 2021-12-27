
	preferredtoolarchitecture "x86_64"

	filter { "platforms:x86" }
		system "Windows"
		architecture "x86"
	filter { "platforms:x86_64" }
		system "Windows"
		architecture "x86_64"
	filter { "platforms:arm" }
		system "Windows"
		architecture "ARM"
	filter { "platforms:arm64" }
		system "Windows"
		architecture "ARM64"
	filter {}
