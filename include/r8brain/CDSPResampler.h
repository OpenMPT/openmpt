//$ nocpp

/**
 * @file CDSPResampler.h
 *
 * @brief The master sample rate converter (resampler) class.
 *
 * This file includes the master sample rate converter (resampler) class that
 * combines all elements of this library into a single front-end class.
 *
 * r8brain-free-src Copyright (c) 2013-2014 Aleksey Vaneev
 * See the "License.txt" file for license.
 */

#ifndef R8B_CDSPRESAMPLER_INCLUDED
#define R8B_CDSPRESAMPLER_INCLUDED

#include "CDSPBlockConvolver.h"
#include "CDSPFracInterpolator.h"

namespace r8b {

/**
 * @brief The master sample rate converter (resampler) class.
 *
 * This class can be considered the "master" sample rate converter (resampler)
 * class since it combines all functionality of this library into a single
 * front-end class to perform sample rate conversion to/from any sample rate,
 * including non-integer sample rates.
 *
 * Note that objects of this class can be constructed on the stack as it has a
 * small member data size. The default template parameters of this class are
 * suited for 27-bit fixed point resampling.
 *
 * Use the CDSPResampler16 class for 16-bit resampling.
 *
 * Use the CDSPResampler16IR class for 16-bit impulse response resampling.
 *
 * Use the CDSPResampler24 class for 24-bit resampling (including 32-bit
 * floating point resampling).
 *
 * @param CInterpClass Interpolator class that should be used by the
 * resampler. The desired interpolation quality can be defined via the
 * template parameters of the interpolator class. See
 * r8b::CDSPFracInterpolator and r8b::CDSPFracDelayFilterBank for description
 * of the template parameters.
 */

template< class CInterpClass =
	CDSPFracInterpolator< R8B_FLTLEN, R8B_FLTFRACS > >
class CDSPResampler : public CDSPProcessor
{
public:
	/**
	 * Constructor initalizes the resampler object.
	 *
	 * Note that increasing the transition band and decreasing attenuation
	 * reduces the filter length, this in turn reduces the "input before
	 * output" delay. However, the filter length has only a minor influence on
	 * the overall resampling speed.
	 *
	 * It should be noted that the ReqAtten specifies the minimal difference
	 * between the loudest input signal component and the produced aliasing
	 * artifacts during resampling. For example, if ReqAtten=100 was specified
	 * when performing 2x upsampling, the analysis of the resulting signal may
	 * display high-frequency components which are quieter than the loudest
	 * part of the input signal by only 100 decibel meaning the high-frequency
	 * part did not become "magically" completely silent after resampling. You
	 * have to specify a higher ReqAtten value if you need a totally clean
	 * high-frequency content. On the other hand, it may not be reasonable to
	 * have a high-frequency content cleaner than the input signal itself: if
	 * the input signal is 16-bit, setting ReqAtten to 150 will make its
	 * high-frequency content 24-bit, but the original part of the signal will
	 * remain 16-bit.
	 *
	 * @param SrcSampleRate Source signal sample rate. Both sample rates can
	 * be specified as a ratio, e.g. SrcSampleRate = 1.0, DstSampleRate = 2.0.
	 * @param DstSampleRate Destination signal sample rate. The "power of 2"
	 * ratios between the source and destination sample rates force resampler
	 * to use several fast "power of 2" resampling steps, without using
	 * fractional interpolation at all. Note that the "power of 2" upsampling
	 * (but not downsampling) requires a lot of buffer memory: e.g. upsampling
	 * by a factor of 16 requires an intermediate buffer MaxInLen*(16+8)
	 * samples long. So, when doing the "power of 2" upsampling it is highly
	 * recommended to do it in small steps, e.g. no more than 256 samples at
	 * once (also set the MaxInLen to 256).
	 * @param MaxInLen The maximal planned length of the input buffer (in
	 * samples) that will be passed to the resampler. The resampler relies on
	 * this value as it allocates intermediate buffers. Input buffers longer
	 * than this value should never be supplied to the resampler. Note that
	 * the resampler may use the input buffer itself for intermediate sample
	 * data storage.
	 * @param ReqTransBand Required transition band, in percent of the
	 * spectral space of the input signal (or the output signal if
	 * downsampling is performed) between filter's -3 dB point and the Nyquist
	 * frequency. The range is from CDSPFIRFilter::getLPMinTransBand() to
	 * CDSPFIRFilter::getLPMaxTransBand(), inclusive. When upsampling 88200 or
	 * 96000 audio to a higher sample rates the ReqTransBand can be
	 * considerably increased, up to 30. The selection of ReqTransBand depends
	 * on the level of desire to preserve the high-frequency content. While
	 * values 0.5 to 2 are extremely "greedy" settings, not necessary in most
	 * cases, values 2 to 3 can be used in most cases. Values 3 to 4 are
	 * relaxed settings, but they still offer a flat frequency response up to
	 * 21kHz with 44.1k source or destination sample rate.
	 * @param ReqAtten Required stop-band attenuation in decibel, in the range
	 * CDSPFIRFilter::getLPMinAtten() to CDSPFIRFilter::getLPMaxAtten(),
	 * inclusive. The actual attenuation may be 0.40-4.46 dB higher. The
	 * general formula for selecting the ReqAtten is 6.02 * Bits + 40, where
	 * "Bits" is the bit resolution (e.g. 16, 24), "40" is an added resolution
	 * for stationary signals, this value can be decreased to 20 to 10 if the
	 * signal being resampled is mostly non-stationary (e.g. impulse
	 * response).
	 * @param ReqPhase Required filter's phase response. Note that this
	 * setting does not affect interpolator's phase response which is always
	 * linear-phase. Also note that if the "power of 2" resampling was engaged
	 * by the resampler together with the minimum-phase response, the audio
	 * stream may become fractionally delayed by up to 1 sample, depending on
	 * the minimum-phase filter's actual fractional delay. If the output
	 * stream should always start at "time zero" offset with minimum-phase
	 * filters the UsePower2 should be set to "false". Linear-phase filters
	 * do not have fractional delay.
	 * @param UsePower2 "True" if the "power of 2" resampling optimization
	 * should be used when possible. This value should be set to "false" if
	 * the access to interpolator is needed in any case (also the source and
	 * destination sample rates should not be equal).
	 * @see CDSPFIRFilterCache::getLPFilter()
	 */

	CDSPResampler( const double SrcSampleRate, const double DstSampleRate,
		const int MaxInLen, const double ReqTransBand = 2.0,
		const double ReqAtten = 206.91,
		const EDSPFilterPhaseResponse ReqPhase = fprLinearPhase,
		const bool UsePower2 = true )
	{
		R8BASSERT( SrcSampleRate > 0.0 );
		R8BASSERT( DstSampleRate > 0.0 );
		R8BASSERT( MaxInLen > 0 );

		if( SrcSampleRate == DstSampleRate )
		{
			ConvCount = 0;
			return;
		}

		int SrcSRMult;
		int SrcSRDiv = 1;
		int MaxOutLen = MaxInLen;
		int ConvBufCapacities[ 2 ];
		double PrevLatencyFrac = 0.0;

		if( DstSampleRate * 2 > SrcSampleRate )
		{
			// Only a single convolver with 2X upsampling is required.

			SrcSRMult = 2;
			const double NormFreq = ( DstSampleRate > SrcSampleRate ? 0.5 :
				0.5 * DstSampleRate / SrcSampleRate );

			Convs[ 0 ] = new CDSPBlockConvolver(
				CDSPFIRFilterCache :: getLPFilter( NormFreq, ReqTransBand,
				ReqAtten, ReqPhase, 2.0 ), 2, 1, 0.0 );

			ConvCount = 1;
			MaxOutLen = Convs[ 0 ] -> getMaxOutLen( MaxOutLen );
			ConvBufCapacities[ 0 ] = MaxOutLen;
			PrevLatencyFrac = Convs[ 0 ] -> getLatencyFrac();

			// Find if the destination to source sample rate ratio is
			// a "power of 2" value.

			int UseConvCount = 1;

			while( true )
			{
				const double TestSR = SrcSampleRate * ( 1 << UseConvCount );

				if( TestSR > DstSampleRate )
				{
					UseConvCount = 0; // Power of 2 not found.
					break;
				}

				if( TestSR == DstSampleRate )
				{
					break; // Power of 2 found.
				}

				UseConvCount++;
			}

			if( UsePower2 && UseConvCount > 0 )
			{
				R8BASSERT( UseConvCount <= ConvCountMax );

				ConvBufCapacities[ 1 ] = 0;
				ConvCount = UseConvCount;
				int i;

				for( i = 1; i < UseConvCount; i++ )
				{
					const double tb = ( i >= 2 ? 45.0 : 34.0 );

					Convs[ i ] = new CDSPBlockConvolver(
						CDSPFIRFilterCache :: getLPFilter( 0.5, tb, ReqAtten,
						ReqPhase, 2.0 ), 2, 1, PrevLatencyFrac );

					MaxOutLen = Convs[ i ] -> getMaxOutLen( MaxOutLen );
					ConvBufCapacities[ i & 1 ] = MaxOutLen;
					PrevLatencyFrac = Convs[ i ] -> getLatencyFrac();
				}

				ConvBufs[ 0 ].alloc( ConvBufCapacities[ 0 ]);

				if( ConvBufCapacities[ 1 ] > 0 )
				{
					ConvBufs[ 1 ].alloc( ConvBufCapacities[ 1 ]);
				}

				return; // No interpolator is needed.
			}

			ConvBufs[ 0 ].alloc( ConvBufCapacities[ 0 ]);
		}
		else
		{
			SrcSRMult = 1;
			ConvBufCapacities[ 0 ] = 0;
			ConvCount = 0;
			const double CheckSR = DstSampleRate * 4;

			while( CheckSR * SrcSRDiv <= SrcSampleRate )
			{
				SrcSRDiv *= 2;

				// If downsampling is even deeper, use a less steep filter at
				// this step.

				const double tb =
					( CheckSR * SrcSRDiv <= SrcSampleRate ? 45.0 : 34.0 );

				Convs[ ConvCount ] = new CDSPBlockConvolver(
					CDSPFIRFilterCache :: getLPFilter( 0.5, tb, ReqAtten,
					ReqPhase, 1.0 ), 1, 2, PrevLatencyFrac );

				MaxOutLen = Convs[ ConvCount ] -> getMaxOutLen( MaxOutLen );
				PrevLatencyFrac = Convs[ ConvCount ] -> getLatencyFrac();
				ConvCount++;

				R8BASSERT( ConvCount < ConvCountMax );
			}

			const double NormFreq = DstSampleRate * SrcSRDiv / SrcSampleRate;
			const int downf = ( UsePower2 && NormFreq == 0.5 ? 2 : 1 );

			Convs[ ConvCount ] = new CDSPBlockConvolver(
				CDSPFIRFilterCache :: getLPFilter( NormFreq, ReqTransBand,
				ReqAtten, ReqPhase, 1.0 ), 1, downf, PrevLatencyFrac );

			MaxOutLen = Convs[ ConvCount ] -> getMaxOutLen( MaxOutLen );
			PrevLatencyFrac = Convs[ ConvCount ] -> getLatencyFrac();
			ConvCount++;

			if( downf > 1 )
			{
				return; // No interpolator is needed.
			}
		}

		Interp = new CInterpClass( SrcSampleRate * SrcSRMult / SrcSRDiv,
			DstSampleRate, PrevLatencyFrac );

		MaxOutLen = Interp -> getMaxOutLen( MaxOutLen );

		if( MaxOutLen <= ConvBufCapacities[ 0 ])
		{
			InterpBuf = ConvBufs[ 0 ];
		}
		else
		if( MaxOutLen <= MaxInLen )
		{
			InterpBuf = NULL;
		}
		else
		{
			TmpBuf.alloc( MaxOutLen );
			InterpBuf = TmpBuf;
		}
	}

	virtual int getLatency() const
	{
		return( 0 );
	}

	virtual double getLatencyFrac() const
	{
		return( 0.0 );
	}

	virtual int getInLenBeforeOutStart( const int NextInLen ) const
	{
		int l = ( Interp == NULL ? 0 :
			Interp -> getInLenBeforeOutStart( NextInLen ));

		int i;

		for( i = ConvCount - 1; i >= 0; i-- )
		{
			l = Convs[ i ] -> getInLenBeforeOutStart( l );
		}

		return( l );
	}

	virtual int getMaxOutLen( const int/* MaxInLen */ ) const
	{
		return( 0 );
	}

	/**
	 * Function clears (resets) the state of *this object and returns it to
	 * the state after construction. All input data accumulated in the
	 * internal buffer so far will be discarded.
	 *
	 * This function makes it possible to use *this object for converting
	 * separate streams from the same source sample rate to the same
	 * destination sample rate without reconstructing the object. It is more
	 * efficient to clear the state of the resampler object than to destroy it
	 * and create a new object.
	 */

	virtual void clear()
	{
		int i;

		for( i = 0; i < ConvCount; i++ )
		{
			Convs[ i ] -> clear();
		}

		if( Interp != NULL )
		{
			Interp -> clear();
		}
	}

	/**
	 * Function performs sample rate conversion.
	 *
	 * If the source and destination sample rates are equal, the resampler
	 * will do nothing and will simply return the input buffer unchanged.
	 *
	 * You do not need to allocate an intermediate output buffer for use with
	 * this function. If required, the resampler will allocate a suitable
	 * intermediate output buffer itself.
	 *
	 * @param ip0 Input buffer. This buffer may be used as output buffer by
	 * this function.
	 * @param l The number of samples available in the input buffer. Should
	 * not exceed the MaxInLen supplied to the constructor.
	 * @param[out] op0 This variable receives the pointer to the resampled
	 * data. On function's return, this pointer may point to the address
	 * within the "ip0" input buffer, or to *this object's internal buffer. In
	 * real-time applications it is suggested to pass this pointer to the next
	 * output audio block and consume any data left from the previous output
	 * audio block first before calling the process() function again. The
	 * buffer pointed to by the "op0" on return may be owned by the resampler,
	 * so it should not be freed by the caller.
	 * @return The number of samples available in the "op0" output buffer. If
	 * the data from the output buffer "op0" is going to be written to a
	 * bigger output buffer, it is suggested to check the returned number of
	 * samples so that no overflow of the bigger output buffer happens.
	 */

	virtual int process( double* ip0, int l, double*& op0 )
	{
		R8BASSERT( l >= 0 );

		if( ConvCount == 0 )
		{
			op0 = ip0;
			return( l );
		}

		double* ip = ip0;
		double* op = NULL;
		int i;

		for( i = 0; i < ConvCount; i++ )
		{
			op = ( ConvBufs[ i & 1 ] == NULL ? ip0 : ConvBufs[ i & 1 ]);
			l = Convs[ i ] -> process( ip, l, op );
			ip = op;
		}

		if( Interp == NULL )
		{
			op0 = op;
			return( l );
		}

		op = ( InterpBuf == NULL ? ip0 : InterpBuf );
		op0 = op;

		return( Interp -> process( ip, l, op ));
	}

	/**
	 * Function performs resampling of an input sample buffer of the specified
	 * length in the "one-shot" mode. This function can be useful when impulse
	 * response resampling is required.
	 *
	 * @param MaxInLen The max input length value which was previously passed
	 * to the constructor.
	 * @param ip Input buffer pointer.
	 * @param iplen Length of the input buffer in samples.
	 * @param op Output buffer pointer.
	 * @param oplen Length of the output buffer in samples.
	 */

	void oneshot( const int MaxInLen, const double* ip, int iplen,
		double* op, int oplen )
	{
		CFixedBuffer< double > ZeroBuf( MaxInLen );
		memset( &ZeroBuf[ 0 ], 0, MaxInLen * sizeof( double ));

		while( oplen > 0 )
		{
			int rc;
			double* p;

			if( iplen == 0 )
			{
				rc = MaxInLen;
				p = (double*) &ZeroBuf[ 0 ];
			}
			else
			{
				rc = min( iplen, MaxInLen );
				p = (double*) ip;
				ip += rc;
				iplen -= rc;
			}

			double* op0;
			int wc = process( p, rc, op0 );

			wc = min( oplen, wc );
			memcpy( op, op0, wc * sizeof( double ));
			op += wc;
			oplen -= wc;
		}

		clear();
	}

private:
	static const int ConvCountMax = 8; ///< 8 convolvers with the
		///< built-in 2x up- or downsampling is enough for 256x up- or
		///< downsampling.
		///<
	CPtrKeeper< CDSPBlockConvolver* > Convs[ ConvCountMax ]; ///< Convolvers.
		///<
	int ConvCount; ///< The number of objects defined in the Convs[] array.
		///< Equals to 0 if sample rate conversion is not needed.
		///<
	CPtrKeeper< CInterpClass* > Interp; ///< Fractional interpolator object.
		///< Equals NULL if no fractional interpolation is required meaning
		///< the "power of 2" resampling is performed or no resampling is
		///< performed at all.
		///<
	CFixedBuffer< double > ConvBufs[ 2 ]; ///< Intermediate convolution
		///< buffers to use, used only when at least 2x upsampling is
		///< performed. These buffers are used in flip-flop manner. If NULL
		///< then the input buffer will be used instead.
		///<
	CFixedBuffer< double > TmpBuf; ///< Additional output buffer, can be
		///< addressed by the InterpBuf pointer.
		///<
	double* InterpBuf; ///< Final output interpolation buffer to use. If NULL
		///< then the input buffer will be used instead. Otherwise this
		///< pointer points to either ConvBufs or TmpBuf.
		///<
};

/**
 * @brief The resampler class for 16-bit resampling.
 *
 * This class defines resampling parameters suitable for 16-bit resampling,
 * using linear-phase low-pass filter. See the r8b::CDSPResampler class for
 * details.
 */

class CDSPResampler16 :
	public CDSPResampler< CDSPFracInterpolator< 18, 137 > >
{
public:
	/**
	 * Constructor initializes the 16-bit resampler. See the
	 * r8b::CDSPResampler class for details.
	 *
	 * @param SrcSampleRate Source signal sample rate.
	 * @param DstSampleRate Destination signal sample rate.
	 * @param MaxInLen The maximal planned length of the input buffer (in
	 * samples) that will be passed to the resampler.
	 * @param ReqTransBand Required transition band, in percent.
	 */

	CDSPResampler16( const double SrcSampleRate, const double DstSampleRate,
		const int MaxInLen, const double ReqTransBand = 2.0 )
		: CDSPResampler< CDSPFracInterpolator< 18, 137 > >( SrcSampleRate,
			DstSampleRate, MaxInLen, ReqTransBand, 136.45, fprLinearPhase,
			true )
	{
	}
};

/**
 * @brief The resampler class for 16-bit impulse response resampling.
 *
 * This class defines resampling parameters suitable for 16-bit impulse
 * response resampling, using linear-phase low-pass filter. Impulse responses
 * usually do not feature stationary signal components and thus need resampler
 * with a less SNR. See the r8b::CDSPResampler class for details.
 */

class CDSPResampler16IR :
	public CDSPResampler< CDSPFracInterpolator< 14, 67 > >
{
public:
	/**
	 * Constructor initializes the 16-bit impulse response resampler. See the
	 * r8b::CDSPResampler class for details.
	 *
	 * @param SrcSampleRate Source signal sample rate.
	 * @param DstSampleRate Destination signal sample rate.
	 * @param MaxInLen The maximal planned length of the input buffer (in
	 * samples) that will be passed to the resampler.
	 * @param ReqTransBand Required transition band, in percent.
	 */

	CDSPResampler16IR( const double SrcSampleRate, const double DstSampleRate,
		const int MaxInLen, const double ReqTransBand = 2.0 )
		: CDSPResampler< CDSPFracInterpolator< 14, 67 > >( SrcSampleRate,
			DstSampleRate, MaxInLen, ReqTransBand, 109.56, fprLinearPhase,
			true )
	{
	}
};

/**
 * @brief The resampler class for 24-bit resampling.
 *
 * This class defines resampling parameters suitable for 24-bit resampling
 * (including 32-bit floating point resampling), using linear-phase low-pass
 * filter. See the r8b::CDSPResampler class for details.
 */

class CDSPResampler24 :
	public CDSPResampler< CDSPFracInterpolator< 24, 673 > >
{
public:
	/**
	 * Constructor initializes the 24-bit resampler (including 32-bit floating
	 * point). See the r8b::CDSPResampler class for details.
	 *
	 * @param SrcSampleRate Source signal sample rate.
	 * @param DstSampleRate Destination signal sample rate.
	 * @param MaxInLen The maximal planned length of the input buffer (in
	 * samples) that will be passed to the resampler.
	 * @param ReqTransBand Required transition band, in percent.
	 */

	CDSPResampler24( const double SrcSampleRate, const double DstSampleRate,
		const int MaxInLen, const double ReqTransBand = 2.0 )
		: CDSPResampler< CDSPFracInterpolator< 24, 673 > >( SrcSampleRate,
			DstSampleRate, MaxInLen, ReqTransBand, 180.15, fprLinearPhase,
			true )
	{
	}
};

} // namespace r8b

#endif // R8B_CDSPRESAMPLER_INCLUDED
