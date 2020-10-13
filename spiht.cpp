// spiht implementations
#include "spiht.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include "general.h"
#include "tbb/tick_count.h"

// encodes separated channels using settings
// and calls appropriate number of singleChannelEncode()
void Spiht::encode(Settings &sets) {
	
	if(sets.printExtended)
		EXTENDED = true;
	else
		EXTENDED = false;
		
	if(sets.printTiming)
		TIMING = true;
	else
		TIMING = false;
	
	if(EXTENDED) {
		std::cout << "-----------------------" << std::endl;
		std::cout << "Base Color Plane Splitter for SPIHT encoding enabled." << std::endl;
	}
	
	// erase bitstream
	dt_.bs_.clear();
	wUnit varY, varCB, varCR;
	elapsedTime_ = 0.0;
	
	tbb::tick_count t0 = tbb::tick_count::now();
	
	varY = imagePtr->computeTotalVariance(sets,y);
	varCB = sets.biasCB * imagePtr->computeTotalVariance(sets,cB);
	varCR = sets.biasCR * imagePtr->computeTotalVariance(sets,cR);
	
	tbb::tick_count t1 = tbb::tick_count::now();
	elapsedTime_ += (t1-t0).seconds();
	
	if(TIMING)
		std::cout << "Variance measurement elapsed time: " <<  std::fixed << std::setprecision(8)  << elapsedTime_ << std::endl; 
	
	
	if(EXTENDED) {
		std::cout << "Total variance for plane y at depth " << sets.varianceDepth << " is " 
				  << varY << std::endl;
		std::cout << "Total variance for plane cB at depth " << sets.varianceDepth << " (biased) is " 
				  << varCB << std::endl;
		std::cout << "Total variance for plane cR at depth " << sets.varianceDepth << " (biased) is " 
				  << varCR << std::endl;
	}

	// finish DWT
	if(!sets.computeDeepVariance && sets.colorShift > 0) {
		double elapsedTemp;
		
		tbb::tick_count t0 = tbb::tick_count::now();
		
		Flwt::forward(sets.colorShift, imagePtr->getMatrix(cB));
		Flwt::forward(sets.colorShift, imagePtr->getMatrix(cR));
		
		tbb::tick_count t1 = tbb::tick_count::now();
		elapsedTemp = (t1-t0).seconds();
		elapsedTime_ += elapsedTemp;
		
		if(TIMING)
			std::cout << "Further WT for CLS elapsed time: " << std::fixed << std::setprecision(8) << elapsedTemp << std::endl; 
		
		if(EXTENDED)
			std::cout << "Performed further forward WT on planes cB, cR by " << sets.colorShift << " levels." << std::endl;
	}

	for(unsigned p = 0; p < 3; p ++) {
		wUnit pct;
		if(p == 0)
			pct = varY / (varY + varCB + varCR);
		else if(p==1)
			pct = varCB / (varY + varCB + varCR);
		else
			pct = varCR / (varY + varCB + varCR);

		unsigned bits = (unsigned) ceil(pct * (wUnit) sets.bits);
		if(EXTENDED)
			std::cout << std::endl << "For plane " << p << " algorithm assigned " << bits << "/" << sets.bits 
				  << " bits (" << std::setprecision(2) << pct * 100.0 << "%)" << std::endl; 
					
		// call for singleChannelEncode
		singleChannelEncode(sets, (planeVal) p, bits);			
	}
}

// decodes separated channels using settings
// and calls appropriate number of singleChannelDecode()
void Spiht::decode(Settings &sets, unsigned desiredBits) {
	
	if(sets.printExtended)
		EXTENDED = true;
	if(sets.printTiming)
		TIMING = true;
		
	if(EXTENDED) {
		std::cout << "-----------------------" << std::endl;
		std::cout << "Base Color Plane Splitter for SPIHT decoding enabled." << std::endl;
	}
	
	// basic condition
	if(dt_.bs_.size() != 3) {
		std::cout << "Splitter Error! BitStream empty or incompatible!" << std::endl;
		throw ExcWrongBitStream();	
	}
	elapsedTime_ = 0.0;
	// clear & init image
	imagePtr->clear(dt_.getWidth(), dt_.getHeight());

	std::cout << std::endl;
	
	std::vector<unsigned> bitCounts(3,0);
		
	unsigned bitSum = dt_.bs_[0].getTotalBits() + dt_.bs_[1].getTotalBits() + dt_.bs_[2].getTotalBits();
	// "ratio-ize" the bitCounts
	if(desiredBits > 0 && desiredBits < bitSum) {
		for(unsigned p = 0; p < 3; p ++)
			bitCounts[p] = (unsigned) floor(((wUnit) dt_.bs_[p].getTotalBits() / (wUnit) bitSum) * (wUnit) bitSum);
	}

	for(unsigned p = 0; p < 3; p ++) {
		// call for singleChannelDecode
		singleChannelDecode(sets, (planeVal) p, bitCounts[p]);			
	}
}