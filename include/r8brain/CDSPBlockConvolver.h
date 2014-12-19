//$ nocpp

/**
 * @file CDSPBlockConvolver.h
 *
 * @brief Single-block overlap-save convolution processor class.
 *
 * This file includes single-block overlap-save convolution processor class.
 *
 * r8brain-free-src Copyright (c) 2013-2014 Aleksey Vaneev
 * See the "License.txt" file for license.
 */

#ifndef R8B_CDSPBLOCKCONVOLVER_INCLUDED
#define R8B_CDSPBLOCKCONVOLVER_INCLUDED

#include "CDSPFIRFilter.h"
#include "CDSPProcessor.h"

namespace r8b {

/**
 * @brief Single-block overlap-save convolution processing class.
 *
 * Class that implements single-block overlap-save convolution processing. The
 * length of a single FFT block used depends on the length of the filter
 * kernel.
 *
 * The rationale behind "single-block" processing is that increasing the FFT
 * block length by 2 is more efficient than performing convolution at the same
 * FFT block length but using two blocks.
 *
 * This class also implements a built-in resampling by any whole-number
 * factor, which simplifies the overall resampling objects topology.
 */

class CDSPBlockConvolver : public CDSPProcessor
{
public:
	/**
	 * Constructor initializes internal variables and constants of *this
	 * object.
	 *
	 * @param aFilter Pre-calculated filter data. Reference to this object is
	 * inhertied by *this object, and the object will be released when *this
	 * object is destroyed. If upsampling is used, filter's gain should be
	 * equal to the upsampling factor.
	 * @param aUpFactor The upsampling factor, positive value. E.g. value of 2
	 * means 2x upsampling should be performed over the input data.
	 * @param aDownFactor The downsampling factor, positive value. E.g. value
	 * of 2 means 2x downsampling should be performed over the output data.
	 * @param PrevLatency Latency, in samples (any value >=0), which was left
	 * in the output signal by a previous process. This value is usually
	 * non-zero if the minimum-phase filters are in use. This value is always
	 * zero if the linear-phase filters are in use.
	 * @param aDoConsumeLatency "True" if the output latency should be
	 * consumed. Does not apply to the fractional part of the latency (if such
	 * part is available).
	 */

	CDSPBlockConvolver( CDSPFIRFilter& aFilter, const int aUpFactor,
		const int aDownFactor, const double PrevLatency = 0.0,
		const bool aDoConsumeLatency = true )
		: Filter( &aFilter )
		, UpFactor( aUpFactor )
		, DownFactor( aDownFactor )
		, DoConsumeLatency( aDoConsumeLatency )
		, BlockLen2( 2 << Filter -> getBlockLenBits() )
	{
		R8BASSERT( UpFactor > 0 );
		R8BASSERT( DownFactor > 0 );
		R8BASSERT( PrevLatency >= 0.0 );

		int fftinBits;
		UpShift = getBitOccupancy( UpFactor ) - 1;

		if(( 1 << UpShift ) == UpFactor )
		{
			fftinBits = Filter -> getBlockLenBits() + 1 - UpShift;
			PrevInputLen = ( Filter -> getKernelLen() - 1 ) / UpFactor;
			InputLen = BlockLen2 - PrevInputLen * UpFactor;
		}
		else
		{
			UpShift = -1;
			fftinBits = Filter -> getBlockLenBits() + 1;
			PrevInputLen = Filter -> getKernelLen() - 1;
			InputLen = BlockLen2 - PrevInputLen;
		}

		OutOffset = Filter -> getLatency();
		LatencyFrac = Filter -> getLatencyFrac() + PrevLatency * UpFactor;
		Latency = (int) LatencyFrac;
		LatencyFrac -= Latency;
		LatencyFrac /= DownFactor;

		Latency += InputLen + OutOffset;

		int fftoutBits;
		InputDelay = 0;
		UpSkipInit = 0;
		DownSkipInit = 0;
		DownShift = getBitOccupancy( DownFactor ) - 1;

		if(( 1 << DownShift ) == DownFactor )
		{
			fftoutBits = Filter -> getBlockLenBits() + 1 - DownShift;

			if( DownFactor > 1 )
			{
				if( UpShift > 0 )
				{
					// This case never happens in practice due to mutual
					// exclusion of "power of 2" DownFactor and UpFactor
					// values.

					R8BASSERT( UpShift == 0 );
				}
				else
				{
					int Delay = Latency & ( DownFactor - 1 );

					if( Delay > 0 )
					{
						Delay = DownFactor - Delay;
						Latency += Delay;

						if( Delay < UpFactor )
						{
							UpSkipInit = Delay;
						}
						else
						{
							UpSkipInit = UpFactor - 1;
							InputDelay = Delay - UpSkipInit;
						}
					}

					if( !DoConsumeLatency )
					{
						Latency /= DownFactor;
					}
				}
			}
		}
		else
		{
			fftoutBits = Filter -> getBlockLenBits() + 1;
			DownShift = -1;

			if( !DoConsumeLatency && DownFactor > 1 )
			{
				DownSkipInit = Latency % DownFactor;
				Latency /= DownFactor;
			}
		}

		fftin = new CDSPRealFFTKeeper( fftinBits );

		if( fftoutBits == fftinBits )
		{
			fftout = fftin;
		}
		else
		{
			ffto2 = new CDSPRealFFTKeeper( fftoutBits );
			fftout = ffto2;
		}

		WorkBlocks.alloc( BlockLen2 * 2 + PrevInputLen );
		CurInput = &WorkBlocks[ 0 ];
		CurOutput = &WorkBlocks[ BlockLen2 ];
		PrevInput = &WorkBlocks[ BlockLen2 * 2 ];

		clear();

		R8BCONSOLE( "CDSPBlockConvolver: flt_len=%i in_len=%i io=%i/%i "
			"fft=%i/%i latency=%i\n", Filter -> getKernelLen(), InputLen,
			UpFactor, DownFactor, (*fftin) -> getLen(), (*fftout) -> getLen(),
			getLatency() );
	}

	virtual ~CDSPBlockConvolver()
	{
		Filter -> unref();
	}

	virtual int getLatency() const
	{
		return( DoConsumeLatency ? 0 : Latency );
	}

	virtual double getLatencyFrac() const
	{
		return( LatencyFrac );
	}

	virtual int getInLenBeforeOutStart( const int NextInLen ) const
	{
		return(( InputLen - InputDelay + NextInLen * DownFactor ) /
			UpFactor );
	}

	virtual int getMaxOutLen( const int MaxInLen ) const
	{
		R8BASSERT( MaxInLen >= 0 );

		return(( MaxInLen * UpFactor + InputDelay + DownFactor - 1 ) /
			DownFactor );
	}

	virtual void clear()
	{
		memset( &PrevInput[ 0 ], 0, PrevInputLen * sizeof( double ));

		if( DoConsumeLatency )
		{
			LatencyLeft = Latency;
		}
		else
		{
			LatencyLeft = 0;

			if( DownShift > 0 )
			{
				memset( &CurOutput[ 0 ], 0, ( BlockLen2 >> DownShift ) *
					sizeof( double ));
			}
			else
			{
				memset( &CurOutput[ BlockLen2 - OutOffset ], 0, OutOffset *
					sizeof( double ));

				memset( &CurOutput[ 0 ], 0, ( InputLen - OutOffset ) *
					sizeof( double ));
			}
		}

		memset( CurInput, 0, InputDelay * sizeof( double ));

		InDataLeft = InputLen - InputDelay;
		UpSkip = UpSkipInit;
		DownSkip = DownSkipInit;
	}

	virtual int process( double* ip, int l0, double*& op0 )
	{
		R8BASSERT( l0 >= 0 );
		R8BASSERT( UpFactor / DownFactor <= 1 || ip != op0 || l0 == 0 );

		double* op = op0;
		int l = l0 * UpFactor;
		l0 = 0;

		while( l > 0 )
		{
			const int Offs = InputLen - InDataLeft;

			if( l < InDataLeft )
			{
				InDataLeft -= l;

				if( UpShift >= 0 )
				{
					memcpy( &CurInput[ Offs >> UpShift ], ip,
						( l >> UpShift ) * sizeof( double ));
				}
				else
				{
					copyUpsample( ip, &CurInput[ Offs ], l );
				}

				copyToOutput( Offs - OutOffset, op, l, l0 );
				break;
			}

			const int b = InDataLeft;
			l -= b;
			InDataLeft = InputLen;
			int ilu;

			if( UpShift >= 0 )
			{
				const int bu = b >> UpShift;
				memcpy( &CurInput[ Offs >> UpShift ], ip,
					bu * sizeof( double ));

				ip += bu;
				ilu = InputLen >> UpShift;
			}
			else
			{
				copyUpsample( ip, &CurInput[ Offs ], b );
				ilu = InputLen;
			}

			const int pil = (int) ( PrevInputLen * sizeof( double ));
			memcpy( &CurInput[ ilu ], PrevInput, pil );
			memcpy( PrevInput, &CurInput[ ilu - PrevInputLen ], pil );

			(*fftin) -> forward( CurInput );

			if( UpShift > 0 )
			{
				mirrorInputSpectrum();
			}

			if( Filter -> isZeroPhase() )
			{
				(*fftout) -> multiplyBlocksZ( Filter -> getKernelBlock(),
					CurInput );
			}
			else
			{
				(*fftout) -> multiplyBlocks( Filter -> getKernelBlock(),
					CurInput );
			}

			if( DownShift > 0 )
			{
				const int z = BlockLen2 >> DownShift;
				CurInput[ 1 ] = Filter -> getKernelBlock()[ z ] *
					CurInput[ z ];
			}

			(*fftout) -> inverse( CurInput );

			copyToOutput( Offs - OutOffset, op, b, l0 );

			double* const tmp = CurInput;
			CurInput = CurOutput;
			CurOutput = tmp;
		}

		return( l0 );
	}

private:
	CDSPFIRFilter* Filter; ///< Filter in use.
		///<
	CPtrKeeper< CDSPRealFFTKeeper* > fftin; ///< FFT object 1, used to produce
		///< the input spectrum (can embed the "power of 2" upsampling).
		///<
	CPtrKeeper< CDSPRealFFTKeeper* > ffto2; ///< FFT object 2 (can be NULL).
		///<
	CDSPRealFFTKeeper* fftout; ///< FFT object used to produce the output
		///< signal (can embed the "power of 2" downsampling), may point to
		///< either "fftin" or "ffto2".
		///<
	int UpFactor; ///< Upsampling factor.
		///<
	int DownFactor; ///< Downsampling factor.
		///<
	bool DoConsumeLatency; ///< "True" if the output latency should be
		///< consumed. Does not apply to the fractional part of the latency
		///< (if such part is available).
		///<
	int BlockLen2; ///< Equals block length * 2.
		///<
	int OutOffset; ///< Output offset, depends on filter's introduced latency.
		///<
	int PrevInputLen; ///< The length of previous input data saved, used for
		///< overlap.
		///<
	int InputLen; ///< The number of input samples that should be accumulated
		///< before the input block is processed.
		///<
	int Latency; ///< Processing latency, in samples.
		///<
	double LatencyFrac; ///< Fractional latency, in samples, that is left in
		///< the output signal.
		///<
	int UpShift; ///< "Power of 2" upsampling shift. Equals -1 if UpFactor is
		///< not a "power of 2" value. Equals 0 if UpFactor equals 1.
		///<
	int DownShift; ///< "Power of 2" downsampling shift. Equals -1 if
		///< DownFactor is not a "power of 2". Equals 0 if DownFactor equals
		///< 1.
		///<
	int InputDelay; ///< Additional input delay, in samples. Used to make the
		///< output latency divisible by DownShift. Used only if UpShift <= 0
		///< and DownShift > 0.
		///<
	CFixedBuffer< double > WorkBlocks; ///< Previous input data, input and
		///< output data blocks, overall capacity = BlockLen2 * 2 +
		///< PrevInputLen. Used in the flip-flop manner.
		///<
	double* PrevInput; ///< Previous input data buffer, capacity = BlockLen.
		///<
	double* CurInput; ///< Input data buffer, capacity = BlockLen2.
		///<
	double* CurOutput; ///< Output data buffer, capacity = BlockLen2.
		///<
	int InDataLeft; ///< Samples left before processing input and output FFT
		///< blocks. Initialized to InputLen on clear.
		///<
	int LatencyLeft; ///< Latency in samples left to skip.
		///<
	int UpSkip; ///< The current upsampling sample skip (value in the range
		///< 0 to UpFactor - 1).
		///<
	int UpSkipInit; ///< The initial UpSkip value after clear().
		///<
	int DownSkip; ///< The current downsampling sample skip (value in the
		///< range 0 to DownFactor - 1). Not used if DownShift > 0.
		///<
	int DownSkipInit; ///< The initial DownSkip value after clear().
		///<

	/**
	 * Function copies samples from the input buffer to the output buffer
	 * while inserting zeros inbetween them to perform the whole-numbered
	 * upsampling.
	 *
	 * @param[in,out] ip0 Input buffer. Will be advanced on function's return.
	 * @param[out] op Output buffer.
	 * @param l0 The number of samples to fill in the output buffer, including
	 * both input samples and interpolation (zero) samples.
	 */

	void copyUpsample( double*& ip0, double* op, int l0 )
	{
		int b = min( UpSkip, l0 );

		if( b > 0 )
		{
			l0 -= b;
			UpSkip -= b;
			*op = 0.0;
			op++;
			b--;

			while( b > 0 )
			{
				*op = 0.0;
				op++;
				b--;
			}
		}

		double* ip = ip0;
		int l = l0 / UpFactor;
		int lz = l0 - l * UpFactor;

		if( UpFactor == 3 )
		{
			while( l > 0 )
			{
				op[ 0 ] = *ip;
				op[ 1 ] = 0.0;
				op[ 2 ] = 0.0;
				ip++;
				op += UpFactor;
				l--;
			}
		}
		else
		if( UpFactor == 5 )
		{
			while( l > 0 )
			{
				op[ 0 ] = *ip;
				op[ 1 ] = 0.0;
				op[ 2 ] = 0.0;
				op[ 3 ] = 0.0;
				op[ 4 ] = 0.0;
				ip++;
				op += UpFactor;
				l--;
			}
		}
		else
		{
			while( l > 0 )
			{
				op[ 0 ] = *ip;
				int j;

				for( j = 1; j < UpFactor; j++ )
				{
					op[ j ] = 0.0;
				}

				ip++;
				op += UpFactor;
				l--;
			}
		}

		if( lz > 0 )
		{
			*op = *ip;
			op++;
			ip++;
			UpSkip = UpFactor - lz;

			while( lz > 1 )
			{
				*op = 0.0;
				op++;
				lz--;
			}
		}

		ip0 = ip;
	}

	/**
	 * Function copies sample data from the CurOutput buffer to the specified
	 * output buffer and advances its position. If necessary, this function
	 * "consumes" latency and performs downsampling.
	 *
	 * @param Offs CurOutput buffer offset, can be negative.
	 * @param[out] op0 Output buffer pointer, will be advanced.
	 * @param b The number of output samples available, including those which
	 * are discarded during whole-number downsampling.
	 * @param l0 The overall output sample count, will be increased.
	 */

	void copyToOutput( int Offs, double*& op0, int b, int& l0 )
	{
		if( Offs < 0 )
		{
			if( Offs + b <= 0 )
			{
				Offs += BlockLen2;
			}
			else
			{
				copyToOutput( Offs + BlockLen2, op0, -Offs, l0 );
				b += Offs;
				Offs = 0;
			}
		}

		if( LatencyLeft > 0 )
		{
			if( LatencyLeft >= b )
			{
				LatencyLeft -= b;
				return;
			}

			Offs += LatencyLeft;
			b -= LatencyLeft;
			LatencyLeft = 0;
		}

		const int df = DownFactor;

		if( DownShift > 0 )
		{
			int Skip = Offs & ( df - 1 );

			if( Skip > 0 )
			{
				Skip = df - Skip;
				b -= Skip;
				Offs += Skip;
			}

			if( b > 0 )
			{
				b = ( b + df - 1 ) >> DownShift;
				memcpy( op0, &CurOutput[ Offs >> DownShift ],
					b * sizeof( double ));

				op0 += b;
				l0 += b;
			}
		}
		else
		{
			if( df > 1 )
			{
				const double* ip = &CurOutput[ Offs + DownSkip ];
				int l = ( b + df - 1 - DownSkip ) / df;
				DownSkip += l * df - b;

				double* op = op0;
				l0 += l;
				op0 += l;

				while( l > 0 )
				{
					*op = *ip;
					op++;
					ip += df;
					l--;
				}
			}
			else
			{
				memcpy( op0, &CurOutput[ Offs ], b * sizeof( double ));
				op0 += b;
				l0 += b;
			}
		}
	}

	/**
	 * Function performs input spectrum mirroring which is used to perform a
	 * fast "power of 2" upsampling. Such mirroring is equivalent to insertion
	 * of zeros into the input signal.
	 */

	void mirrorInputSpectrum()
	{
		const int bl1 = BlockLen2 >> UpShift;
		const int bl2 = bl1 + bl1;

		int i;

		for( i = bl1 + 2; i < bl2; i += 2 )
		{
			CurInput[ i ] = CurInput[ bl2 - i ];
			CurInput[ i + 1 ] = -CurInput[ bl2 - i + 1 ];
		}

		CurInput[ bl1 ] = CurInput[ 1 ];
		CurInput[ bl1 + 1 ] = 0.0;
		CurInput[ 1 ] = CurInput[ 0 ];

		for( i = 1; i < UpShift; i++ )
		{
			const int z = bl1 << i;
			memcpy( &CurInput[ z ], CurInput, z * sizeof( double ));
			CurInput[ z + 1 ] = 0.0;
		}
	}
};

} // namespace r8b

#endif // R8B_CDSPBLOCKCONVOLVER_INCLUDED
