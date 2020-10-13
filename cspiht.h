// CSPIHT declaration file
#ifndef CSPIHT_H
#define CSPIHT_H

#include "general.h"
#include "image.h"
#include "settings.h"
#include "colorcodec.h"
#include <list>
#include <iostream>

// class CSPIHT
// Color SPIHT implementation by a class
class CSpiht : public ColorCodec {
	const static unsigned char version = 0xA0;

	// struct for X,Y,P coordinate storage
	struct XYP {
		wCoord X;
		wCoord Y;
		planeVal P;
		XYP(): X(0), Y(0), P(y) {};
		explicit XYP(wCoord x, wCoord y, planeVal p): X(x), Y(y), P(p) {};
			
	};

	// struct for X,Y,P,type coordinate storage
	struct XYPT {
		wCoord X;
		wCoord Y;
		planeVal P;
		typeVal T;
		XYPT(): X(0), Y(0), P(y), T(typeA) {};
		explicit XYPT(wCoord x, wCoord y, planeVal p, typeVal t): X(x), Y(y), P(p), T(t) {};
	};

	// on-the-fly properties
	int n_;					// current step
	unsigned nMax_;			// max steps
	wUnit currThr_;			// current threshold
	wUnit halfThr_;			// half threshold, used only in decoding
	bool decodingOver_;		// flag for decoding is over

	// lists
	std::list<XYPT> LIS_;
	std::list<XYP> LIP_;
	std::list<XYP> LSP_;

	// ----------- private methods
	// init LIS members - put root nodes in
	// init LIP members - put 
	void initLists();
	// computes max val of image, returns maxSteps property
	unsigned computeSteps();
	// (encoding) does a sorting pass, output enabled, returns number of bits outputted
	unsigned sortingPassC(DataGroup::BitStream &bs);
	// (encoding) does a refinement pass, output enabled, returns number of bits outputted
	unsigned refinementPassC(DataGroup::BitStream &bs);
	// recursive tree significance searcher - see implementation for usage notes
	bool checkSignificance(wCoord X, wCoord Y, planeVal P, bool startNow);
	// (decoding) does a sorting pass, returns number of bits processed
	unsigned sortingPassD(DataGroup::BitStream &bs);
	// (decoding) does a refinement pass, returns number of bits processed
	unsigned refinementPassD(DataGroup::BitStream &bs);
	
	// image &ref
	Image &image;

public:
	// constructor with add. params
	CSpiht(Image& im);
	// encode wrapper
	virtual void encode(Settings &sets);
	// decode wrapper
	virtual void decode(Settings &sets, unsigned desiredBits=0);
};

#endif
