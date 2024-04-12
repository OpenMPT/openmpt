  
 project "opus"
  uuid "9a2d9099-e1a2-4287-b845-e3598ad24d70"
  language "C"
  location ( "%{wks.location}" .. "/ext" )
  mpt_kind "default"
  targetname "openmpt-opus"
	includedirs {
		"../../include/opus/include",
		"../../include/opus/celt",
		"../../include/opus/dnn",
		"../../include/opus/silk",
		"../../include/opus/silk/fixed",
		"../../include/opus/silk/float",
		"../../include/opus/src",
		"../../include/opus/win32",
		"../../include/opus",
	}
	filter {}
		files {
			"../../include/opus/include/opus.h",
			"../../include/opus/include/opus_custom.h",
			"../../include/opus/include/opus_defines.h",
			"../../include/opus/include/opus_multistream.h",
			"../../include/opus/include/opus_projection.h",
			"../../include/opus/include/opus_types.h",
		}
		files {
			"../../include/opus/celt/*.c",
			"../../include/opus/celt/*.h",
			"../../include/opus/silk/*.c",
			"../../include/opus/silk/*.h",
			"../../include/opus/silk/float/*.c",
			"../../include/opus/silk/float/*.h",
			"../../include/opus/src/*.c",
			"../../include/opus/src/*.h",
		}
	filter { "architecture:x86 or x86_64" }
		files {
			"../../include/opus/celt/x86/*.c",
			"../../include/opus/celt/x86/*.h",
			"../../include/opus/silk/float/x86/*.c",
			"../../include/opus/silk/float/x86/*.h",
			"../../include/opus/silk/x86/*.c",
			"../../include/opus/silk/x86/*.h",
		}
	filter {}
	filter { "architecture:arm or arm64" }
		files {
			"../../include/opus/celt/arm/*.c",
			"../../include/opus/celt/arm/*.h",
			"../../include/opus/silk/float/arm/*.c",
			"../../include/opus/silk/float/arm/*.h",
			"../../include/opus/silk/arm/*.c",
			"../../include/opus/silk/arm/*.h",
		}
	filter {}
		excludes {
			"../../include/opus/celt/opus_custom_demo.c",
			"../../include/opus/src/opus_compare.c",
			"../../include/opus/src/opus_demo.c",
			"../../include/opus/src/repacketizer_demo.c",
		}
	filter {}
		defines {
			"OPUS_BUILD=1",
			"PACKAGE_VERISON=\"1.5.2\"",
			"ENABLE_HARDENING=1",
			"HAVE_STDINT_H=1",
			"HAVE_STDIO_H=1",
			"HAVE_STDLIB_H=1",
			"HAVE_STRING_H=1",
			"USE_ALLOCA=1",
		}
	filter {}
		if false then -- NoLACE (OSCE) / LACE (OSCE) / DEEP-PLC (DEEP_PLC || DRED) / DRED (DRED).
			filter {}
				files {
					"../../include/opus/dnn/*.c",
					"../../include/opus/dnn/*.h",
				}
				defines {
					"ENABLE_DEEP_PLC=1",
					"ENABLE_DRED=1",
					"ENABLE_OSCE=1",
				}
			filter {}
			filter { "architecture:x86 or x86_64" }
				files {
					"../../include/opus/dnn/x86/*.c",
					"../../include/opus/dnn/x86/*.h",
				}
			filter {}
			filter { "architecture:arm or arm64" }
				files {
					"../../include/opus/dnn/arm/*.c",
					"../../include/opus/dnn/arm/*.h",
				}
			filter {}
				excludes {
					"../../include/opus/dnn/dump_data.c",
					"../../include/opus/dnn/fargan_demo.c",
					"../../include/opus/dnn/lossgen.c",
					"../../include/opus/dnn/lossgen_data.c",
					"../../include/opus/dnn/lossgen_demo.c",
					"../../include/opus/dnn/write_lpcnet_weights.c",
				}
			filter {}
		end
	filter {}
	if _OPTIONS["clang"] then
		filter { "architecture:x86" }
			defines {
				"OPUS_HAVE_RTCD=1",
				"CPU_INFO_BY_C=1",
			}
			excludes {
				"../../include/opus/dnn/x86/nnet_avx2.c",
				"../../include/opus/dnn/x86/nnet_sse4_1.c",
			}
		filter {}
		filter { "architecture:x86_64" }
			defines {
				"OPUS_HAVE_RTCD=1",
				"CPU_INFO_BY_C=1",
				"OPUS_X86_MAY_HAVE_SSE=1",
				"OPUS_X86_MAY_HAVE_SSE2=1",
				"OPUS_X86_PRESUME_SSE=1",
				"OPUS_X86_PRESUME_SSE2=1",
			}
			excludes {
				"../../include/opus/dnn/x86/nnet_avx2.c",
				"../../include/opus/dnn/x86/nnet_sse4_1.c",
			}
		filter {}
		filter { "architecture:arm" }
			excludes {
				"../../include/opus/celt/arm/celt_fft_ne10.c",
				"../../include/opus/celt/arm/celt_mdct_ne10.c",
				"../../include/opus/celt/arm/celt_neon_intr.c",
				"../../include/opus/celt/arm/pitch_neon_intr.c",
				"../../include/opus/dnn/arm/nnet_dotprod.c",
				"../../include/opus/dnn/arm/nnet_neon.c",
			}
		filter {}
		filter { "architecture:arm64" }
			excludes {
				"../../include/opus/celt/arm/celt_fft_ne10.c",
				"../../include/opus/celt/arm/celt_mdct_ne10.c",
				"../../include/opus/celt/arm/celt_neon_intr.c",
				"../../include/opus/celt/arm/pitch_neon_intr.c",
				"../../include/opus/dnn/arm/nnet_dotprod.c",
				"../../include/opus/dnn/arm/nnet_neon.c",
			}
		filter {}
	else
		if _OPTIONS["windows-version"] == "winxp" then
			filter { "architecture:x86" }
				defines {
					"OPUS_HAVE_RTCD=1",
					"CPU_INFO_BY_C=1",
					"OPUS_X86_MAY_HAVE_SSE=1",
					"OPUS_X86_MAY_HAVE_SSE2=1",
					"OPUS_X86_MAY_HAVE_SSE4_1=1",
				}
			filter {}
		else
			filter { "architecture:x86" }
			defines {
				"OPUS_HAVE_RTCD=1",
				"CPU_INFO_BY_C=1",
				"OPUS_X86_MAY_HAVE_SSE=1",
				"OPUS_X86_MAY_HAVE_SSE2=1",
				"OPUS_X86_MAY_HAVE_SSE4_1=1",
			}
			filter {}
			filter { "architecture:x86", "configurations:Checked" }
				defines {
					"OPUS_X86_PRESUME_SSE",
					"OPUS_X86_PRESUME_SSE2",
				}
			filter {}
			filter { "architecture:x86", "configurations:CheckedShared" }
				defines {
					"OPUS_X86_PRESUME_SSE",
					"OPUS_X86_PRESUME_SSE2",
				}
			filter {}
			filter { "architecture:x86", "configurations:Release" }
				defines {
					"OPUS_X86_PRESUME_SSE",
					"OPUS_X86_PRESUME_SSE2",
				}
			filter {}
			filter { "architecture:x86", "configurations:ReleaseShared" }
				defines {
					"OPUS_X86_PRESUME_SSE",
					"OPUS_X86_PRESUME_SSE2",
				}
			filter {}
		end
		filter {}
		filter { "architecture:x86_64" }
			defines {
				"OPUS_HAVE_RTCD=1",
				"CPU_INFO_BY_C=1",
				"OPUS_X86_MAY_HAVE_SSE=1",
				"OPUS_X86_MAY_HAVE_SSE2=1",
				"OPUS_X86_MAY_HAVE_SSE4_1=1",
				"OPUS_X86_MAY_HAVE_AVX2=1",
				"OPUS_X86_PRESUME_SSE",
				"OPUS_X86_PRESUME_SSE2",
			}
		filter {}
		filter { "architecture:arm" }
			excludes {
				"../../include/opus/celt/arm/celt_fft_ne10.c",
				"../../include/opus/celt/arm/celt_mdct_ne10.c",
				"../../include/opus/celt/arm/celt_neon_intr.c",
				"../../include/opus/celt/arm/pitch_neon_intr.c",
				"../../include/opus/dnn/arm/nnet_dotprod.c",
				"../../include/opus/dnn/arm/nnet_neon.c",
			}
			defines {
				--"OPUS_HAVE_RTCD=1",
				--"CPU_INFO_BY_C=1",
				--"OPUS_ARM_MAY_HAVE_DOTPROD=1",
				--"OPUS_ARM_MAY_HAVE_EDSP=1",
				--"OPUS_ARM_MAY_HAVE_MEDIA=1",
				--"OPUS_ARM_MAY_HAVE_NEON=1",
				--"OPUS_ARM_MAY_HAVE_NEON_INTR=1",
			}
		filter {}
		filter { "architecture:arm64" }
			excludes {
				"../../include/opus/celt/arm/celt_fft_ne10.c",
				"../../include/opus/celt/arm/celt_mdct_ne10.c",
				"../../include/opus/celt/arm/celt_neon_intr.c",
				"../../include/opus/celt/arm/pitch_neon_intr.c",
				"../../include/opus/dnn/arm/nnet_dotprod.c",
				"../../include/opus/dnn/arm/nnet_neon.c",
			}
			defines {
				--"OPUS_HAVE_RTCD=1",
				--"CPU_INFO_BY_C=1",
				--"OPUS_ARM_MAY_HAVE_DOTPROD=1",
				--"OPUS_ARM_MAY_HAVE_EDSP=1",
				--"OPUS_ARM_MAY_HAVE_MEDIA=1",
				--"OPUS_ARM_MAY_HAVE_NEON=1",
				--"OPUS_ARM_MAY_HAVE_NEON_INTR=1",
				--"OPUS_ARM_PRESUME_NEON_INTR=1",
			}
		filter {}
	end
	links { }
	filter { "action:vs*" }
		buildoptions {
			"/wd4244",
			"/wd4305",
		}
	filter {}
	filter { "action:vs*" }
		buildoptions { -- analyze
			"/wd6255",
			"/wd6297",
		}
	filter {}
		if _OPTIONS["clang"] then
			buildoptions {
				"-Wno-excess-initializers",
				"-Wno-macro-redefined",
			}
		end
	filter {}
	filter { "kind:SharedLib" }
		defines { "DLL_EXPORT" }
	filter {}
		if _OPTIONS["clang"] then
			defines { "FLOAT_APPROX" }
		end

function mpt_use_opus ()
	filter {}
	filter { "action:vs*" }
		includedirs {
			"../../include/opus/include",
		}
	filter { "not action:vs*" }
		externalincludedirs {
			"../../include/opus/include",
		}
	filter {}
	links {
		"opus",
	}
	filter {}
end
