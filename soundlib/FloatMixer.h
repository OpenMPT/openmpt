// FIXME left vs right
// FIXME: Playing samples backwards should reverse interpolation LUTs as well - we might need separate LUTs because otherwise we will add tons of branches. Done for linear interpolation.
// Konzept loop-wraparound:
// Hinter dem eigentlichen Sample Platz lassen für 2x max LUT size * num channels * samplesize
// Die letzten LUT samples vor dem loopend und die ersten LUT samples nach dem loopstart werden dorthin kopiert
// -> Bzw bei pingpong-samples die letzten LUT samples umdrehen, statt die ersten LUT samples zu verwenden
// Je nachdem, wo wir uns im sample befinden, beim rendern dann das normale sample oder den loopteil hinter dem sample als pointer verwenden
// -> Auch für pingpong-samples am samplestart? 

typedef float mixsample_t;

extern mixsample_t gFastSincf[];
extern mixsample_t gKaiserSincf[];		// 8-taps polyphase
extern mixsample_t gDownsample13xf[];	// 1.3x downsampling
extern mixsample_t gDownsample2xf[];	// 2x downsampling
extern mixsample_t gLinearInterpolationForward[];
extern mixsample_t gLinearInterpolationBackward[];

//////////////////////////////////////////////////////////////////////////
// Sample conversion traits

template<int channelsOut, int channelsIn, typename out, typename in>
struct BasicTraits
{
	static const int numChannelsIn = channelsIn;	// Number of channels in sample
	static const int numChannelsOut = channelsOut;	// Number of mixer output channels
	typedef out output_t;							// Output buffer sample type
	typedef in input_t;								// Input buffer sample type
	typedef out outbuf_t[channelsOut];				// Output buffer sampling point type
	// To perform sample conversion, add a function with the following signature to your derived classes:
	// static forceinline output_t Convert(const input_t x)
};

template<int channelsOut, int channelsIn, typename out, typename in, int int2float>
struct IntToFloatTraits : public BasicTraits<channelsOut, channelsIn, out, in>
{
	static_assert(std::numeric_limits<input_t>::is_integer, "Input must be integer");
	static_assert(!std::numeric_limits<output_t>::is_integer, "Output must be floating point");

	static forceinline output_t Convert(const input_t x)
	{
		return static_cast<output_t>(x) * (static_cast<output_t>(1.0f) / static_cast<output_t>(int2float));
	}
};

typedef IntToFloatTraits<2, 1, mixsample_t, int8,  -int8_min>  Int8MToFloatS;
typedef IntToFloatTraits<2, 1, mixsample_t, int16, -int16_min> Int16MToFloatS;
typedef IntToFloatTraits<2, 2, mixsample_t, int8,  -int8_min>  Int8SToFloatS;
typedef IntToFloatTraits<2, 2, mixsample_t, int16, -int16_min> Int16SToFloatS;


//////////////////////////////////////////////////////////////////////////
// Interpolation templates

template<class Traits>
struct NoInterpolation
{
	forceinline void Start(const ModChannel &) { }
	forceinline void End(const ModChannel &) { }

	forceinline void operator() (typename Traits::outbuf_t &outSample, const typename Traits::input_t * const inBuffer, const int32)
	{
		static_assert(Traits::numChannelsIn <= Traits::numChannelsOut, "Too many input channels");

		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			outSample[i] = Traits::Convert(inBuffer[i]);
		}
	}
};


template<class Traits>
struct LinearInterpolation
{
	const mixsample_t *lut;
	int dir;

	forceinline void Start(const ModChannel &chn)
	{
		lut = chn.nInc >= 0 ? gLinearInterpolationForward : gLinearInterpolationBackward;
		dir = chn.nInc >= 0 ? Traits::numChannelsIn : -Traits::numChannelsIn;
	}

	forceinline void End(const ModChannel &) { }

	forceinline void operator() (typename Traits::outbuf_t &outSample, const typename Traits::input_t * const inBuffer, const int32 posLo)
	{
		static_assert(Traits::numChannelsIn <= Traits::numChannelsOut, "Too many input channels");
		const mixsample_t fract = lut[posLo >> 8];

		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			Traits::output_t srcVol = Traits::Convert(inBuffer[i]);
			Traits::output_t destVol = Traits::Convert(inBuffer[i + dir]);

			outSample[i] = srcVol + fract * (destVol - srcVol);
		}
	}
};


template<class Traits>
struct FastSincInterpolation
{
	forceinline void Start(const ModChannel &) { }
	forceinline void End(const ModChannel &) { }

	forceinline void operator() (typename Traits::outbuf_t &outSample, const typename Traits::input_t * const inBuffer, const int32 posLo)
	{
		static_assert(Traits::numChannelsIn <= Traits::numChannelsOut, "Too many input channels");
		const mixsample_t *lut = gFastSincf + ((posLo >> 6) & 0x3FC);

		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			outSample[i] =
				  lut[0] * Traits::Convert(inBuffer[i - Traits::numChannelsIn])
				+ lut[1] * Traits::Convert(inBuffer[i])
				+ lut[2] * Traits::Convert(inBuffer[i + Traits::numChannelsIn])
				+ lut[3] * Traits::Convert(inBuffer[i + 2 * Traits::numChannelsIn]);
		}
	}
};


template<class Traits>
struct PolyphaseInterpolation
{
	const mixsample_t *sinc;
	forceinline void Start(const ModChannel &chn)
	{
		sinc = (((chn.nInc > 0x13000) || (chn.nInc < -0x13000)) ?
			(((chn.nInc > 0x18000) || (chn.nInc < -0x18000)) ? gDownsample2xf : gDownsample13xf) : gKaiserSincf);
	}

	forceinline void End(const ModChannel &) { }

	forceinline void operator() (typename Traits::outbuf_t &outSample, const typename Traits::input_t * const inBuffer, const int32 posLo)
	{
		static_assert(Traits::numChannelsIn <= Traits::numChannelsOut, "Too many input channels");
		const mixsample_t *lut = sinc + ((posLo >> 1) & ~0x1F);

		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			outSample[i] =
				  lut[0] * Traits::Convert(inBuffer[i - 3 * Traits::numChannelsIn])
				+ lut[1] * Traits::Convert(inBuffer[i - 2 * Traits::numChannelsIn])
				+ lut[2] * Traits::Convert(inBuffer[i - Traits::numChannelsIn])
				+ lut[3] * Traits::Convert(inBuffer[i])
				+ lut[4] * Traits::Convert(inBuffer[i + Traits::numChannelsIn])
				+ lut[5] * Traits::Convert(inBuffer[i + 2 * Traits::numChannelsIn])
				+ lut[6] * Traits::Convert(inBuffer[i + 3 * Traits::numChannelsIn])
				+ lut[7] * Traits::Convert(inBuffer[i + 4 * Traits::numChannelsIn]);
		}
	}
};


template<class Traits>
struct FIRFilterInterpolation
{
	forceinline void Start(const ModChannel &) { }
	forceinline void End(const ModChannel &) { }

	forceinline void operator() (typename Traits::outbuf_t &outSample, const typename Traits::input_t * const inBuffer, const int32 posLo)
	{
		static_assert(Traits::numChannelsIn <= Traits::numChannelsOut, "Too many input channels");
		const mixsample_t * const lut = CWindowedFIR::lutf + (((posLo + WFIR_FRACHALVE) >> WFIR_FRACSHIFT) & WFIR_FRACMASK);

		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			outSample[i] =
				  lut[0] * Traits::Convert(inBuffer[i - 3 * Traits::numChannelsIn])
				+ lut[1] * Traits::Convert(inBuffer[i - 2 * Traits::numChannelsIn])
				+ lut[2] * Traits::Convert(inBuffer[i - Traits::numChannelsIn])
				+ lut[3] * Traits::Convert(inBuffer[i])
				+ lut[4] * Traits::Convert(inBuffer[i + Traits::numChannelsIn])
				+ lut[5] * Traits::Convert(inBuffer[i + 2 * Traits::numChannelsIn])
				+ lut[6] * Traits::Convert(inBuffer[i + 3 * Traits::numChannelsIn])
				+ lut[7] * Traits::Convert(inBuffer[i + 4 * Traits::numChannelsIn]);
		}
	}
};


//////////////////////////////////////////////////////////////////////////
// Mixing templates (add sample to stereo mix)

template<class Traits>
struct NoRamp
{
	typename Traits::output_t lVol, rVol;

	forceinline void Start(const ModChannel &chn)
	{
		lVol = static_cast<Traits::output_t>(chn.nLeftVol) * (1.0f / 4096.0f);
		rVol = static_cast<Traits::output_t>(chn.nRightVol) * (1.0f / 4096.0f);
	}

	forceinline void End(const ModChannel &) { }
};


struct Ramp
{
	int32 lRamp, rRamp;

	forceinline void Start(const ModChannel &chn)
	{
		lRamp = chn.nRampLeftVol;
		rRamp = chn.nRampRightVol;
	}

	forceinline void End(ModChannel &chn)
	{
		chn.nRampLeftVol = lRamp; chn.nLeftVol = lRamp >> VOLUMERAMPPRECISION;
		chn.nRampRightVol = rRamp; chn.nRightVol = rRamp >> VOLUMERAMPPRECISION;
	}
};


// Legacy optimization: If chn.nLeftVol == chn.nRightVol, save one multiplication instruction
template<class Traits>
struct MixMonoFastNoRamp : public NoRamp<Traits>
{
	forceinline void operator() (const typename Traits::outbuf_t &outSample, const ModChannel &chn, typename Traits::output_t * const outBuffer)
	{
		Traits::output_t vol = outSample[0] * lVol;
		for(int i = 0; i < Traits::numChannelsOut; i++)
		{
			outBuffer[i] += vol;
		}
	}
};


template<class Traits>
struct MixMonoNoRamp : public NoRamp<Traits>
{
	forceinline void operator() (const typename Traits::outbuf_t &outSample, const ModChannel &, typename Traits::output_t * const outBuffer)
	{
		outBuffer[0] += outSample[0] * lVol;
		outBuffer[1] += outSample[0] * rVol;
	}
};


template<class Traits>
struct MixMonoRamp : public Ramp
{
	forceinline void operator() (const typename Traits::outbuf_t &outSample, const ModChannel &chn, typename Traits::output_t * const outBuffer)
	{
		// TODO volume is not float!
		lRamp += chn.nLeftRamp;
		rRamp += chn.nRightRamp;
		outBuffer[0] += outSample[0] * (lRamp >> VOLUMERAMPPRECISION) * (1.0f / 4096.0f);
		outBuffer[1] += outSample[0] * (rRamp >> VOLUMERAMPPRECISION) * (1.0f / 4096.0f);
	}
};


template<class Traits>
struct MixStereoNoRamp : public NoRamp<Traits>
{
	forceinline void operator() (const typename Traits::outbuf_t &outSample, const ModChannel &, typename Traits::output_t * const outBuffer)
	{
		outBuffer[0] += outSample[0] * lVol;
		outBuffer[1] += outSample[1] * rVol;
	}
};


template<class Traits>
struct MixStereoRamp : public Ramp
{
	forceinline void operator() (const typename Traits::outbuf_t &outSample, const ModChannel &chn, typename Traits::output_t * const outBuffer)
	{
		// TODO volume is not float!
		lRamp += chn.nLeftRamp;
		rRamp += chn.nRightRamp;
		outBuffer[0] += outSample[0] * (lRamp >> VOLUMERAMPPRECISION) * (1.0f / 4096.0f);
		outBuffer[1] += outSample[1] * (rRamp >> VOLUMERAMPPRECISION) * (1.0f / 4096.0f);
	}
};


//////////////////////////////////////////////////////////////////////////
// Filter templates


template<class Traits>
struct NoFilter
{
	forceinline void Start(const ModChannel &) { }
	forceinline void End(const ModChannel &) { }

	forceinline void operator() (const typename Traits::outbuf_t &, const ModChannel &) { }
};


// Resonant filter
template<class Traits>
struct ResonantFilter
{
	// Filter history
	typename Traits::output_t fy[Traits::numChannelsIn][2];

	forceinline void Start(const ModChannel &chn)
	{
		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			fy[i][0] = chn.nFilter_Y[i][0];
			fy[i][1] = chn.nFilter_Y[i][1];
		}
	}

	forceinline void End(ModChannel &chn)
	{
		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			chn.nFilter_Y[i][0] = fy[i][0];
			chn.nFilter_Y[i][1] = fy[i][1];
		}
	}

	// Filter values are clipped to double the input range
#define ClipFilter(x) Clamp(x, static_cast<Traits::output_t>(-2.0f), static_cast<Traits::output_t>(2.0f))

	forceinline void operator() (typename Traits::outbuf_t &outSample, const ModChannel &chn)
	{
		static_assert(Traits::numChannelsIn <= Traits::numChannelsOut, "Too many input channels");

		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			Traits::output_t val = (outSample[i] * chn.nFilter_A0 + ClipFilter(fy[i][0]) * chn.nFilter_B0 + ClipFilter(fy[i][1]) * chn.nFilter_B1);
			fy[i][1] = fy[i][0];
			fy[i][0] = val - (outSample[i] * chn.nFilter_HPf);
			outSample[i] = val;
		}
	}

#undef ClipFilter
};


//////////////////////////////////////////////////////////////////////////
// Main sample render loop template


// Template parameters:
// Traits: A BasicTraits instance that defines the number of channels, sample buffer types, etc..
// InterpolationFunc: Functor for reading the sample data and doing the SRC
// FilterFunc: Functor for applying the resonant filter
// MixFunc: Functor for mixing the computed sample data into the output buffer
template<class Traits, class InterpolationFunc, class FilterFunc, class MixFunc>
static void MPPASMCALL SampleLoop(ModChannel &chn, typename Traits::output_t *outBuffer, int numSamples)
{
	register ModChannel &c = chn;
	const Traits::input_t *inSample = static_cast<const Traits::input_t *>(c.pCurrentSample) + c.nPos * Traits::numChannelsIn;
	int32 smpPos = c.nPosLo;	// 16.16 sample position relative to c.nPos

	InterpolationFunc interpolate;
	FilterFunc filter;
	MixFunc mix;

	// Do initialisation if necessary
	interpolate.Start(c);
	filter.Start(c);
	mix.Start(c);

	register int samples = numSamples;
	do
	{
		Traits::outbuf_t outSample;
		interpolate(outSample, inSample + (smpPos >> 16) * Traits::numChannelsIn, (smpPos & 0xFFFF));
		filter(outSample, c);
		mix(outSample, c, outBuffer);
		outBuffer += Traits::numChannelsOut;

		smpPos += c.nInc;
	} while(--samples);

	mix.End(c);
	filter.End(c);
	interpolate.End(c);

	c.nPos += smpPos >> 16;
	c.nPosLo = smpPos & 0xFFFF;
};
