// Flwt class - defines CDF 9/7 forward and inverse DTWT over referenced Matrix 
// Class should be called static only
#ifndef FLWT_H
#define FLWT_H

#include "general.h"

// this class performs a 2D-DWT on a Matrix<wUnit>
// using CDF 9/7 fast lifting scheme transform
class Flwt {
	// direct transform performers
	static void rowTransformF(Matrix<wUnit> &source, unsigned W, unsigned H);
	static void columnTransformF(Matrix<wUnit> &source, unsigned W, unsigned H);
	static void rowTransformI(Matrix<wUnit> &source, unsigned W, unsigned H);
	static void columnTransformI(Matrix<wUnit> &source, unsigned W, unsigned H);
public:
	// interface
	static void forward(unsigned level, Matrix<wUnit> & matrix);
	static void inverse(unsigned level, Matrix<wUnit> & matrix);

	//// static properties
	//static unsigned lastBandSizeW;
	//static unsigned lastBandSizeH;
	//static unsigned lastLevel;
};

#endif
