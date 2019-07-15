// FilterFracs formula fitting optimizer.

#include <stdio.h>
#include "/projects/biteopt/biteopt.h"

const int DefFracCount = 13;
static double SNR[ DefFracCount ] = { 51.7280, 67.1095, 81.8379, 96.4021,
	111.1300, 125.4649, 139.7378, 154.0532, 168.2101, 182.1076, 195.5668,
	209.0609, 222.5009 };

static const int DefFracs[ DefFracCount ] = { 11, 17, 27, 47, 77, 115, 190,
	257, 471, 759, 1311, 1870, 3190 };

const int OptDepth = 9;
bool DoPrint = false;

class CFracFnOpt : public CBiteOptDeep
{
public:
	CFracFnOpt()
	{
		updateDims( 2, OptDepth );
	}

	virtual void getMinValues( double* const p ) const
	{
		p[ 0 ] = 0.0;
		p[ 1 ] = 0.0001;
	}

	virtual void getMaxValues( double* const p ) const
	{
		p[ 0 ] = 2.0;
		p[ 1 ] = 1.0;
	}

	virtual double optcost( const double* const p )
	{
		double s = 0.0;
		int i;

		for( i = 0; i < DefFracCount; i++ )
		{
			const int FracCount = (int) ceil( p[ 0 ] *
				exp( p[ 1 ] * SNR[ i ]));

			const double d = log( (double) FracCount ) -
				log( (double) DefFracs[ i ]);

			if( DoPrint )
			{
				printf( "%i %i\n", FracCount, DefFracs[ i ]);
			}

			s += d * d;
		}

		return( s );
	}
};

int main()
{
	CBiteRnd rnd;
	rnd.init( 1 );

	CFracFnOpt opt;
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

	printf( "bestcost %.15f\n%.15f %.15f\n", opt.getBestCost(),
		opt.getBestParams()[ 0 ], opt.getBestParams()[ 1 ]);

	DoPrint = true;
	opt.optcost( opt.getBestParams() );
}
