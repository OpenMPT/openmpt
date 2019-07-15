// Window function parameters optimizer - finds parameters for the specified
// filter length for Kaiser window functions using the BiteOptDeep
// derivative-free optimization method.

#include <stdio.h>
#include <math.h>
#include "/projects/biteopt/biteopt.h"
#include "../CDSPSincFilterGen.h"

const double CornerFreq = 1.0; // Corner frequency.
const double LinFraction = 0.5; // Linear part of the spectrum.
//const double LinFraction = 1.0 / 3.0; // Linear part of the spectrum one-third.
const double StopFraction = 2.0 - LinFraction; // Stop-band part of the spectrum.
const double StopFractionEnd = 4.0; // Stop-band part of the spectrum end.
int FilterLen = 24; // Filter length.
const int Oversample = 20; // Spectrum oversampling ratio.
const int OptDepth = 9; // BiteOptDeep optimizer depth.
bool DoPrintLin = false; // Print max linear part non-linearity?

class CWinOpt : public CBiteOptDeep
{
public:
	CWinOpt()
	{
		updateDims( 2, OptDepth );
	}

	virtual void getMinValues( double* const p ) const
	{
		p[ 0 ] = 2.0;
		p[ 1 ] = 0.5;
	}

	virtual void getMaxValues( double* const p ) const
	{
		p[ 0 ] = 30.0;
		p[ 1 ] = 3.0;
	}

	virtual double optcost( const double* const p )
	{
		r8b :: CDSPSincFilterGen gen;
		gen.Freq1 = 0.0;
		gen.Freq2 = M_PI * CornerFreq / Oversample;
		gen.Len2 = FilterLen * 0.5 * Oversample;
		gen.initBand( r8b :: CDSPSincFilterGen :: wftKaiser, p, true );

		const int KernelLen = gen.KernelLen;
		double KernelBlock[ KernelLen ];
		gen.generateBand( &KernelBlock[ 0 ],
			&r8b :: CDSPSincFilterGen :: calcWindowKaiser );

		r8b :: normalizeFIRFilter( &KernelBlock[ 0 ], KernelLen, 1.0 );

		const double _10ln10 = 10.0 / log( 10.0 );
		const int Count1 = 500;
		double cost1 = 0.0;
		double re;
		double im;
		int i;

		for( i = 0; i <= Count1; i++ )
		{
			const double th = M_PI * LinFraction / Oversample * i / Count1;
			r8b :: calcFIRFilterResponse( KernelBlock, KernelLen, th, re, im );
			double p = fabs( _10ln10 * log( re * re + im * im ));

			cost1 = max( p, cost1 );
		}

		if( DoPrintLin )
		{
			printf( "%.4f\n", cost1 );
		}

		const int Count2 = 2000;
		double cost2 = -1000.0;
		const double th1 = M_PI * StopFraction / Oversample;
		const double th2 = M_PI * StopFractionEnd / Oversample;

		for( i = 0; i <= Count2; i++ )
		{
			const double th = th1 + ( th2 - th1 ) * i / Count2;
			r8b :: calcFIRFilterResponse( KernelBlock, KernelLen, th, re, im );
			double p = _10ln10 * log( re * re + im * im );

			cost2 = max( p, cost2 );
		}

		return( cost1 * 36.0 + cost2 );
	}
};

int main()
{
	CBiteRnd rnd;
	rnd.init( 1 );

	CWinOpt opt;

	printf( "\t\tstatic const double Coeffs[][ 3 ] = {\n" );

	for( FilterLen = 6; FilterLen <= 30; FilterLen += 2 )
	{
		opt.init( rnd );

		const int sct = 15 * opt.getInitEvals() / OptDepth; // Plateau thresh.
		int i;

		for( i = 0; i < 1000000; i++ )
		{
			if( opt.optimize( rnd ) > sct )
			{
				break;
			}
		}

		printf( "\t\t\t{ %.16f, %.16f, %.4f }, // ",
			opt.getBestParams()[ 0 ], opt.getBestParams()[ 1 ],
			-opt.getBestCost() );

		DoPrintLin = true;
		opt.optcost( opt.getBestParams() );
		DoPrintLin = false;
	}

	printf( "\t\t};\n" );
}
