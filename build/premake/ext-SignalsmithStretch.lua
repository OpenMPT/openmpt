
function mpt_use_signalsmith_stretch ()
	-- header-only library
	filter {}
	includedirs {
		"../../include/SignalsmithStretch",
	}
	files {
		"../../include/SignalsmithStretch/signalsmith-linear/fft.h",
		"../../include/SignalsmithStretch/signalsmith-linear/stft.h",
		"../../include/SignalsmithStretch/SignalsmithStretch/signalsmith-stretch.h",
	}
	filter {}
end
