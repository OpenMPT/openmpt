#include <stdafx.h>
#include <string.h>
#include <math.h>
#include "FFTReal.h"

/* ============================================================= */
/* nFFTSize must be a power of 2 */
/* ============================================================= */
/* Usage examples: */
/* - suppress the center: fAmpL = 1.f, fAmpC = 0.f, fAmpR = 1.f */
/* - keep only the center: fAmpL = 0.f, fAmpC = 1.f, fAmpR = 0.f */
/* ============================================================= */

void processTBIsolator(float *pIns[2], float *pOuts[2], long nFFTSize, float fAmpL, float fAmpC, float fAmpR)
{
	float fModL, fModR;
	float fRealL, fRealC, fRealR;
	float fImagL, fImagC, fImagR;
	double u;

	for ( long i = 0, j = nFFTSize / 2; i < nFFTSize / 2; i++, j++ )
	{
		fModL = pIns[0][i] * pIns[0][i] + pIns[0][j] * pIns[0][j];
		fModR = pIns[1][i] * pIns[1][i] + pIns[1][j] * pIns[1][j];

		// min on complex numbers
		if ( fModL > fModR )
		{
			fRealC = pIns[1][i];
			fImagC = pIns[1][j];
		}
		else
		{
			fRealC = pIns[0][i];
			fImagC = pIns[0][j];
		}

		// phase correction...
		u = fabs(atan2(pIns[0][j], pIns[0][i]) - atan2(pIns[1][j], pIns[1][i])) / 3.141592653589;

		if ( u >= 1 ) u = 2 - u;

		u = pow(1 - u*u*u, 24);

		fRealC *= (float) u;
		fImagC *= (float) u;

		// center extraction...
		fRealL = pIns[0][i] - fRealC;
		fImagL = pIns[0][j] - fImagC;

		fRealR = pIns[1][i] - fRealC;
		fImagR = pIns[1][j] - fImagC;

		// You can do some treatments here...

		pOuts[0][i] = fRealL * fAmpL + fRealC * fAmpC;
		pOuts[0][j] = fImagL * fAmpL + fImagC * fAmpC;

		pOuts[1][i] = fRealR * fAmpR + fRealC * fAmpC;
		pOuts[1][j] = fImagR * fAmpR + fImagC * fAmpC;
	}
}
