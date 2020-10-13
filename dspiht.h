// BSPIHT: implementation of SPIHT single channel
#ifndef DSPIHT_H
#define DSPIHT_H

#include "colorcodec.h"
#include "spiht.h"
#include "general.h"
#include "image.h"
#include <list>

// class DSPIHT
// this is a degraded-trees version of base SPIHT
// constructs implementation, takes Image
// implements singleChannelEncode & singleChannelDecode
class DSpiht : public Spiht {
	const static unsigned char version = 0xB1;

	// privates
	Image& image;
	planeVal plane_;

	// on-the-fly properties
	int		 n_;			// current step
	unsigned nMax_;			// max steps
	wUnit currThr_;			// current threshold
	wUnit halfThr_;			// half threshold, used only in decoding
	bool decodingOver_;		// flag for decoding is over

	// lists
	std::list<XYT> LIS_;
	std::list<XY> LIP_;
	std::list<XY> LSP_;

	// ----------- private methods
	// init LIS, LIP members
	void initLists();
	// computes max val of image, returns maxSteps property
	unsigned computeSteps();
	// (encoding) does a sorting pass, output enabled, returns number of bits outputted
	unsigned sortingPassC(DataGroup::BitStream &bs);
	// (encoding) does a refinement pass, output enabled, returns number of bits outputted
	unsigned refinementPassC(DataGroup::BitStream &bs);
	// recursive tree significance searcher - see implementation for usage notes
	bool checkSignificance(wCoord X, wCoord Y, bool startNow);
	// (decoding) does a sorting pass, returns number of bits processed
	unsigned sortingPassD(DataGroup::BitStream &bs);
	// (decoding) does a refinement pass, returns number of bits processed
	unsigned refinementPassD(DataGroup::BitStream &bs);

public:
	// constructor
	DSpiht(Image& im);
	// virtual overloads
	virtual void singleChannelEncode(Settings &sets, planeVal p, unsigned bits);
	virtual void singleChannelDecode(Settings &sets, planeVal p, unsigned bits);
};

#endif
