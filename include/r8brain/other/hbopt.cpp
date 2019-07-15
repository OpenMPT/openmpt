// Half-band filter coefficient optimizer - finds coefficients for the
// specified number of taps using the BiteOptDeep derivative-free optimization
// method.

#include <stdio.h>
#include <math.h>
#include "/projects/biteopt/biteopt.h"
#include "../CDSPSincFilterGen.h"

const double Fraction = 4.0; // 1/Fraction of frequency response that should
	// be preserved (in low freqs) and attenuated (in high freqs).
const double TapMult = 2.0; // Tap multiplier.
const int TapCount = 11; // The number of non-zero taps in the filter.
const int KernelLen = TapCount * 4 - 1;
const int KernelLenD2 = KernelLen >> 1;
double Sinc[ TapCount ]; // Sinc function.
const int OptDepth = 9; // BiteOptDeep optimizer depth.

class CHBOpt : public CBiteOptDeep
{
public:
	CHBOpt()
	{
		updateDims( TapCount, OptDepth );
	}

	virtual void getMinValues( double* const p ) const
	{
		int i;

		for( i = 0; i < TapCount; i++ )
		{
			p[ i ] = 0.0;
		}
	}

	virtual void getMaxValues( double* const p ) const
	{
		int i;

		for( i = 0; i < TapCount; i++ )
		{
			p[ i ] = 1.0;
		}
	}

	virtual double optcost( const double* const p )
	{
		double KernelBlock[ KernelLen ] = { 0.0 };
		double* kc = KernelBlock + KernelLenD2;
		*kc = 0.5;
		int i;

		for( i = 0; i < TapCount; i++ )
		{
			kc[ -i * 2 - 1 ] = Sinc[ i ] * p[ i ];
			kc[ i * 2 + 1 ] = Sinc[ i ] * p[ i ];
		}

		const double _10ln10 = 10.0 / log( 10.0 );
		const int Count1 = 250;
		double cost1 = 0.0;
		double re;
		double im;
		const double fr = 1.0 / Fraction;

		for( i = 0; i <= Count1; i++ )
		{
			const double th = M_PI * fr * i / Count1;
			r8b :: calcFIRFilterResponse( KernelBlock, KernelLen, th, re, im );
			double p = fabs( _10ln10 * log( re * re + im * im ));

			cost1 = max( p, cost1 );
		}

		const int Count2 = 500;
		double cost2 = -1000.0;

		for( i = 0; i <= Count2; i++ )
		{
			const double th = M_PI * ( 1.0 - fr * i / Count2 );
			r8b :: calcFIRFilterResponse( KernelBlock, KernelLen, th, re, im );
			double p = _10ln10 * log( re * re + im * im );

			cost2 = max( p, cost2 );
		}

		return( cost1 * 3600.0 + cost2 );
	}
};

int main()
{
	CBiteRnd rnd;
	rnd.init( 1 );

	CHBOpt opt;
	opt.init( rnd );

	int i;

	for( i = 0; i < TapCount; i++ )
	{
		const double xpi = ( i * 2 + 1 ) * M_PI;
		Sinc[ i ] = sin( xpi * 0.5 ) / xpi;
	}

	const int sct = 15 * opt.getInitEvals() / OptDepth; // Plateau threshold.

	for( i = 0; i < 1000000; i++ )
	{
		if( opt.optimize( rnd ) > sct )
		{
			break;
		}
	}

	printf( "static const double HBKernel_%i[ %i ] = { "
		"// att %.4f dB, frac %.1f\n", TapCount, TapCount,
		opt.getBestCost(), Fraction );

	for( i = 0; i < TapCount; i++ )
	{
		printf( "%.16e, ", Sinc[ i ] * opt.getBestParams()[ i ] * TapMult );

		if(( i & 1 ) != 0 )
		{
			printf( "\n" );
		}
	}

	printf( "};\n" );
}
