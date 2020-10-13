// base class for single-channel SPIHT's
// implements encode and decode
// contains separation rules,
// proposes interface for inheritants

#ifndef SPIHT_H
#define SPIHT_H

#include "colorcodec.h"
#include "general.h"
#include "image.h"
#include "settings.h"
#include "flwt.h"

// class Spiht declaration
class Spiht : public ColorCodec {

// to be inherited
protected:
	// image, instead of reference this will be ptr
	// WARNING! must be initialized in inheritant's constructor
	Image *imagePtr;

public:
	// inherited datatypes
	// struct for X,Y,P coordinate storage
	struct XY {
		wCoord X;
		wCoord Y;
		XY(): X(0), Y(0) {};
		explicit XY(wCoord x, wCoord y): X(x), Y(y) {};
			
	};

	// struct for X,Y,type coordinate storage
	struct XYT {
		wCoord X;
		wCoord Y;
		typeVal T;
		XYT(): X(0), Y(0), T(typeA) {};
		explicit XYT(wCoord x, wCoord y, typeVal t): X(x), Y(y), T(t) {};
	};

	// inherited interface
	virtual void encode(Settings &sets);
	virtual void decode(Settings &sets, unsigned desiredBits=0);
	
	// proposed interface
	virtual void singleChannelEncode(Settings &sets, planeVal p, unsigned bits) = 0;
	virtual void singleChannelDecode(Settings &sets, planeVal p, unsigned bits) = 0;
};

#endif