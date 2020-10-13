// BSpiht implementation
#include "bspiht.h"
#include <cmath>
#include <iostream>
#include <iomanip>
#include "tbb/tick_count.h"

// BSpiht constructor
BSpiht::BSpiht(Image &im) : image(im) {
	elapsedTime_ = 0.0;
	// call dataGroupInit
	dt_.DataGroupInit(version, 3, image.getWidth(), image.getHeight());
	// pass image to imagePtr
	imagePtr = &image;
}

// encode function
// get settings, planeVal and bits
// perform encoding of plane
// IMPORTANT ASSUMPTION: this will be called in proper order - y, cB, cR!!!
// otherwise can throw ExcOutOfOrder
void BSpiht::singleChannelEncode(Settings &sets, planeVal p, unsigned bits) {
	if(sets.printExtended)
		EXTENDED = true;
	else
		EXTENDED = false;
		
	if(sets.printTiming)
		TIMING = true;
	else
		TIMING = false;

	// compute bandsizes
	computeBandSize(sets, image, p);

	plane_ = p;
	// test if out of order
	if(p > 2) {
		std::cout << "Wrong plane ID!" << std::endl;
		throw ExcWrongPlaneID();;
	}
	
	// delete all lists
	LIS_.clear();
	LSP_.clear();
	LIP_.clear();
	
	// timer ON
	tbb::tick_count t0 = tbb::tick_count::now();
	
	// init lists
	initLists();
	// get nMax
	nMax_ = computeSteps();
	
	// timer OFF
	tbb::tick_count t1 = tbb::tick_count::now();
	elapsedTime_ += (t1-t0).seconds();
	
	if(TIMING)
		std::cout << std::fixed << std::setprecision(8) << "Elapsed time on init = " << (t1-t0).seconds() << std::endl;
	
	// init params & bs
	n_ = nMax_;
	currThr_ = pow(2.0, (wUnit) nMax_);
		
	dt_.bs_.push_back(ColorCodec::DataGroup::BitStream(nMax_, bits, (p==y)?sets.levels:(sets.levels+sets.colorShift)));
	// ref to bitstream: is now bs
	ColorCodec::DataGroup::BitStream& bs = dt_.bs_[plane_];
	
	if(EXTENDED)
		std::cout << "BSPIHT encoder enabled. Encoding plane " << p << "." << std::endl;
	
	// main loop
	while(n_ >= 0) {
		unsigned currStep = nMax_ - n_ + 1;

		// timer ON
		tbb::tick_count t0 = tbb::tick_count::now();

		unsigned sout = sortingPassC(bs);
		unsigned rout = refinementPassC(bs);

		// timer OFF
		tbb::tick_count t1 = tbb::tick_count::now();
		elapsedTime_ += (t1-t0).seconds();

		if(EXTENDED) 
			std::cout << ((bs.finished)?"F":"S") << std::setw(2) << currStep <<  ", bits=" 
				  << std::setw(6) << sout << "sp + " << std::setw(6) << rout << "rp (" 
				  << std::setw(7) << std::setprecision(1) << std::fixed << (double) (sout+rout) / 8.0 << "B) | "
				  << "LIS: " << std::setw(5) << LIS_.size() <<  ", LIP: " << std::setw(5) << LIP_.size() 
				  << ", LSP: " << std::setw(5) << LSP_.size() << std::endl;

		// possible ending - lossless
		if(n_ == 0) {
			bs.performClose();
		}

		// detect possible ending
		if(bs.finished) {
			if(EXTENDED)
				std::cout << "BSPIHT encoding done. " << bits << " bits (" << std::setprecision(1) << std::fixed
					  << (double) bits/8.0 << "B) stored in bitstream." << std::endl;
			break; 
		}

		n_--; currThr_ /= 2.0; 
	}
}

// decode function
// get settings, planeVal
// perform decoding of plane
// IMPORTANT ASSUMPTION: this will be called in proper order - y, cB, cR!!!
// otherwise can throw ExcOutOfOrder
void BSpiht::singleChannelDecode(Settings &sets, planeVal p, unsigned bits) {
	// compute bandsizes
	computeBandSize(sets, image, p);
	
	if(sets.printExtended)
		EXTENDED = true;
	else
		EXTENDED = false;
	
	if(sets.printTiming)
		TIMING = true;
	else
		TIMING = false;
		
	// test if out of order
	if(p > 2) {
		std::cout << "Wrong plane ID!" << std::endl;
		throw ExcWrongPlaneID();
	}
	
	dt_.DataGroupCheck(version, 3);
	
	// ref to bitstream: is now bs
	plane_ = p;
	ColorCodec::DataGroup::BitStream& bs = dt_.bs_[plane_];
	
	// check if this bs is OK, deal with bitsize
	unsigned bitCnt = bs.checkSettings(sets, bits);
	
	// delete all lists
	LIS_.clear();
	LSP_.clear();
	LIP_.clear();
	
	// timer ON
	tbb::tick_count t0 = tbb::tick_count::now();
	
	// init lists
	initLists();
	nMax_ = bs.getMaxSteps();
	
	// timer OFF
	tbb::tick_count t1 = tbb::tick_count::now();
	elapsedTime_ += (t1-t0).seconds();
	
	if(TIMING)
		std::cout << std::fixed << std::setprecision(8) << "Elapsed time on init = " << (t1-t0).seconds() << std::endl;
	
	// init params & bs
	n_ = nMax_;
	decodingOver_ = false;
	currThr_ = pow(2.0, (wUnit) nMax_);
	halfThr_ = currThr_ / 2.0;	
	
	if(EXTENDED)
		std::cout << "BSPIHT decoder enabled. Decoding plane " << p << "." << std::endl;
	
	// main loop
	while(n_ >= 0) {
		unsigned currStep = nMax_ - n_ + 1;

		// timer ON
		tbb::tick_count t0 = tbb::tick_count::now();

		unsigned sout = sortingPassD(bs);
		unsigned rout = refinementPassD(bs);

		// timer OFF
		tbb::tick_count t1 = tbb::tick_count::now();
		elapsedTime_ += (t1-t0).seconds();

		if(EXTENDED)
			std::cout << ((decodingOver_)?"F":"S") << std::setw(2) << currStep <<  ", bits=" 
				  << std::setw(6) << sout << "sp + " << std::setw(6) << rout << "rp (" 
				  << std::setw(7) << std::setprecision(1) << std::fixed << (double) (sout+rout) / 8.0 << "B) | "
				  << "LIS: " << std::setw(5) << LIS_.size() <<  ", LIP: " << std::setw(5) << LIP_.size() 
				  << ", LSP: " << std::setw(5) << LSP_.size() << std::endl;

		// possible ending - lossless
		if(n_ == 0) {
			bs.performClose();
		}

		// detect possible ending
		if(decodingOver_) {
			if(EXTENDED)
				std::cout << "BSPIHT decoding done. " << bitCnt << " bits (" << std::setprecision(1) << std::fixed
					  << (double) bitCnt/8.0 << "B) from bitstream have been processed." << std::endl;
			break; 
		}

		n_--; currThr_ /= 2.0; halfThr_ /= 2.0;
	}
}
 
 
// ----------- private methods
// init LIS members - put root nodes in
// init LIP members - put 
void BSpiht::initLists() {
	// init LIP. LIP contains all pixels from LLtop.
	// also init LIS. LIS contains only 3/4 of each quadgroup from LLtop.
	for(wCoord j=0; j < bandSizeH_; ++j) {
		for(wCoord i=0; i < bandSizeW_; ++i) {
			LIP_.push_back(XY(i,j));
			if(!(i % 2 == 0 && j % 2 == 0))
				LIS_.push_back(XYT(i,j,typeA));
		}
	}
	
}
// computes max val of image plane and steps number
unsigned BSpiht::computeSteps() {
	wUnit max = image.getMax(plane_);
	
	// compute nMax_
	return (unsigned) floor(log2(max));
}

// coding: does a sorting pass, output enabled, returns number of bits outputted
unsigned BSpiht::sortingPassC(DataGroup::BitStream &bs) {
	unsigned bitsOut = 0;

	// part 1: LIP processing
	std::list<XY>::iterator LIPit = LIP_.begin();

	while(LIPit != LIP_.end()) {
		// backup iterator: fetch current item into it, move to the next
		std::list<XY>::iterator LIPcurr = LIPit++;
		// check for significance
		if(abs(image(LIPcurr->X, LIPcurr->Y, plane_)) >= currThr_) {
			// output 1
			if(!bs.put(1)) return bitsOut; else bitsOut++;
			// output sign
			if(!bs.put(image(LIPcurr->X, LIPcurr->Y, plane_) >= 0.0)) return bitsOut; else bitsOut++;
			// move into LSP
			LSP_.push_back(XY(LIPcurr->X, LIPcurr->Y));
			// delete from LIP
			LIP_.erase(LIPcurr);
		} else {
			// output 0
			if(!bs.put(0)) return bitsOut; else bitsOut++;
		}
	}

	// part 2: LIS processing
	std::list<XYT>::iterator LISit = LIS_.begin();
	while(LISit != LIS_.end()) {
		// backup iterator: fetch current item into it, move to the next
		std::list<XYT>::iterator LIScurr = LISit;

		// check significance
		if(checkSignificance(LIScurr->X, LIScurr->Y, (LIScurr->T == typeA) ? true : false)) {
			// output 1
			if(!bs.put(1)) return bitsOut; else bitsOut++;
			
			// init base coordinates
			wCoord baseX = LIScurr->X; wCoord baseY = LIScurr->Y;
			
			// detect special cases - only 3 possible (3/4 quadtree)
			if(baseX < bandSizeW_ && baseY < bandSizeH_) {
				// LLtop: top-right node
				if(baseY % 2 == 0 && baseX % 2 != 0) {
					baseX += bandSizeW_ - 1;
				// LLtop: bottom-left node
				} else if(baseY % 2 != 0 && baseX % 2 == 0) {
					baseY += bandSizeH_ - 1;
				// LLtop: bottom-right node. 
				} else {
					baseX += bandSizeW_ - 1;	baseY += bandSizeH_ - 1;
				}
			} else {
				// regular quad-tree
				baseX *= 2; baseY *= 2;
			}
	
			// check four descendants directly
			for(int i = 1; i < 5; ++i) {
				// change base coordinates to match corner of the quadgroup
				if(i == 2) {
					baseX++;
				} else if(i == 3) {
					baseX--; baseY++;
				} else if(i == 4) {
					baseX++;
				} 

				// process typeA
				if(LIScurr->T == typeA) {
					// test for significance (single-element)
					if(abs(image(baseX, baseY, plane_)) >= currThr_) {
						// output 1
						if(!bs.put(1)) return bitsOut; else bitsOut++;
						// output sign
						if(!bs.put(image(baseX, baseY, plane_) >= 0.0)) return bitsOut; else bitsOut++;
						// move into LSP
						LSP_.push_back(XY(baseX,baseY));
					} else {
						// output 0
						if(!bs.put(0)) return bitsOut; else bitsOut++;
						// move to LIP
						LIP_.push_back(XY(baseX,baseY));
					}

				// process typeB
				} else {
					// partitioning
					LIS_.push_back(XYT(baseX, baseY, typeA));
				}
			}

			// possible typeB entry creation
			if(LIScurr->T == typeA) {
				// check if image allows more descendants
				if(baseX*2 < (wCoord) image.getWidth() && baseY*2 < (wCoord) image.getHeight()) {
					// put into LIS as entry type B
					LIS_.push_back(XYT(LIScurr->X, LIScurr->Y, typeB));
				}
			}

			// partitioning done, discard LIS entry
			// IMPORTANT / iterate before discard (new ones might be added)
			LISit++;
			LIS_.erase(LIScurr);

		} else {
			// output 0
			if(!bs.put(0)) return bitsOut; else bitsOut++;
			LISit++;
		}
	}

	return bitsOut;
}
// coding: does a refinement pass, output enabled, returns number of bits outputted
unsigned BSpiht::refinementPassC(DataGroup::BitStream &bs) {
	unsigned bitsOut = 0;
	// LSP processing	
	std::list<XY>::iterator LSPit = LSP_.begin();

	// force last time threshold
	wUnit lastThr = pow(2.0, (double) nMax_ - n_ + 1);
	unsigned compare = (unsigned) pow(2.0 , (double) nMax_ + 2);

	while(LSPit != LSP_.end()) {
		unsigned value = (unsigned) floor( abs(image(LSPit->X, LSPit->Y, plane_)) * lastThr );
		// check if ready for transmission
		if(value < compare)
			break;
		if(value & (1 << (nMax_ + 1))) {
			if(!bs.put(1)) return bitsOut; else bitsOut++;
		} else {
			if(!bs.put(0)) return bitsOut; else bitsOut++;
		}
		LSPit++;
	}

	return bitsOut;
}
// recursive tree significance searcher
// compares descendants against the current threshold value
// if max(abs(... detected anywhere in the tree, just bail out with true without more checking
// params: x, y, p - where do we start - desc. will be checked
// t - put true if you want to start right now, with false it will not check the first round (typeB entry)
bool BSpiht::checkSignificance(wCoord X, wCoord Y, bool startNow) {
	// define starting check size
	wCoord size = 2;
	// define descendants base coords
	wCoord baseX; wCoord baseY;
	
	// search for descendants
	if(X < bandSizeW_ && Y < bandSizeH_) {
		// inside LLtop: compute LLtop part root node coords
		baseX = (wCoord) floor((wUnit) X / 2.0) * 2;
		baseY = (wCoord) floor((wUnit) Y / 2.0) * 2;

		// determine correct descendants
		if(Y % 2 == 0) {
			// 2) top right
			baseX = baseX + bandSizeW_;
		} else {
			if(X % 2 == 0) {
				// 3) bottom left
				baseY = baseY + bandSizeH_;
			} else {
				// 4) bottom right
				baseX = baseX + bandSizeW_; baseY = baseY + bandSizeH_;
			}
		}
	} else {
		// normal quadtree position
		baseX = 2*X; baseY = 2*Y;
	}

	// loop to most possible depth
	do {
		// search for significance
		if(startNow) {
			if(image.maxTest(baseX, baseY, size, plane_, currThr_)) return true; 
		} else {
			startNow = true;
		}
		
		// perform loop iteration
		size *= 2; baseX *= 2; baseY *= 2;

	// test against size condition
	} while(baseX < (wCoord) image.getWidth() && baseY < (wCoord) image.getHeight());

	// not found
	return false;
}

// decoding: does a sorting pass, returns number of bits processed
unsigned BSpiht::sortingPassD(DataGroup::BitStream &bs) {
	unsigned bitsOut = 0;
	signed char getBit = 0;

	// part 1: LIP processing
	std::list<XY>::iterator LIPit = LIP_.begin();
	while(LIPit != LIP_.end()) {
		// backup iterator: fetch current item into it, move to the next
		std::list<XY>::iterator LIPcurr = LIPit++;

		// read a bit
		if((getBit = bs.get()) == -1) { decodingOver_ = true; return bitsOut; }
		bitsOut++;

		// check for significance
		if(getBit == 1) {
			// get sign
			if((getBit = bs.get()) == -1) { decodingOver_ = true; return bitsOut; }
			bitsOut++;
			// output to image according to sign
			if(getBit == 1) 
				image(LIPcurr->X, LIPcurr->Y, plane_) = currThr_ + halfThr_;
			else
				image(LIPcurr->X, LIPcurr->Y, plane_) = -1.0 * (currThr_ + halfThr_);

			// move into LSP
			LSP_.push_back(XY(LIPcurr->X, LIPcurr->Y));
			// delete from LIP
			LIP_.erase(LIPcurr);
		}
	}

	// part 2: LIS processing
	std::list<XYT>::iterator LISit = LIS_.begin();
	while(LISit != LIS_.end()) {
		// backup iterator: fetch current item into it, move to the next
		std::list<XYT>::iterator LIScurr = LISit;

		// read a bit
		if((getBit = bs.get()) == -1) { decodingOver_ = true; return bitsOut; }
		bitsOut++;
		// check significance
		if(getBit == 1) {
			// init base coordinates
			wCoord baseX = LIScurr->X; wCoord baseY = LIScurr->Y;
			
			// detect special cases
			if(baseX < bandSizeW_ && baseY < bandSizeH_) {
				// LLtop: top-right node
				if(baseY % 2 == 0 && baseX % 2 != 0) {
					baseX += bandSizeW_ - 1;
				// LLtop: bottom-left node
				} else if(baseY % 2 != 0 && baseX % 2 == 0) {
					baseY += bandSizeH_ - 1;
				// LLtop: bottom-right node. 
				} else {
					baseX += bandSizeW_ - 1;	baseY += bandSizeH_ - 1;
				}
			} else {
				// regular quad-tree
				baseX *= 2; baseY *= 2;
			}
	
			// check four descendants directly
			for(int i = 1; i < 5; ++i) {
				// change base coordinates to match corner of the quadgroup
				if(i == 2) {
					baseX++;
				} else if(i == 3) {
					baseX--; baseY++;
				} else if(i == 4) {
					baseX++;
				} 

				// process typeA
				if(LIScurr->T == typeA) {
					// read a bit
					if((getBit = bs.get()) == -1) { decodingOver_ = true; return bitsOut; }
					bitsOut++;
					// test for significance (single-element)
					if(getBit == 1) {
						// get sign
						if((getBit = bs.get()) == -1) { decodingOver_ = true; return bitsOut; }
						bitsOut++;
						// output to image according to sign
						if(getBit == 1) 
							image(baseX, baseY, plane_) = currThr_ + halfThr_;
						else
							image(baseX, baseY, plane_) = -1.0 * (currThr_ + halfThr_);
						
						// move into LSP
						LSP_.push_back(XY(baseX,baseY));
					} else {
						// move to LIP
						LIP_.push_back(XY(baseX,baseY));
					}

				// process typeB
				} else {
					// partitioning
					LIS_.push_back(XYT(baseX, baseY, typeA));
				}
			}

			// possible typeB entry creation
			if(LIScurr->T == typeA) {
				// check if image allows more descendants
				if(baseX*2 < (wCoord) image.getWidth() && baseY*2 < (wCoord) image.getHeight()) {
					// put into LIS as entry type B
					LIS_.push_back(XYT(LIScurr->X, LIScurr->Y, typeB));
				}
			}

			// partitioning done, discard LIS entry
			// IMPORTANT / iterate before discard (new ones might be added)
			LISit++;
			LIS_.erase(LIScurr);

		} else {
			// iter only
			LISit++;
		}
	}

	return bitsOut;
}
// decoding: does a refinement pass, returns number of bits processed
unsigned BSpiht::refinementPassD(DataGroup::BitStream &bs) {
	// exit upon finished reading
	if(decodingOver_)
		return 0;

	unsigned bitsOut = 0;
	signed char getBit = 0;
	// LSP processing iterator
	std::list<XY>::iterator LSPit = LSP_.begin();

	// force last time threshold
	wUnit lastThr = pow(2.0, (double) n_ - 1);

	// limit for loop
	wUnit limit = pow(2.0, (double) n_ + 1);

	// read bits, "refine" pixels in image marked by LSP
	while(LSPit != LSP_.end()) {
		
		// prepare value
		wUnit value = image(LSPit->X, LSPit->Y, plane_);
		if(abs(value) <= limit)
			break;

		// get a bit
		if((getBit = bs.get()) == -1) { decodingOver_ = true; return bitsOut; }
		bitsOut++;
		
		if(getBit == 1) {
			// positive add
			value = value + lastThr * ((image(LSPit->X, LSPit->Y, plane_) > 0.0) ? 1.0 : -1.0); 
		} else {
			// negative add
			value = value - lastThr * ((image(LSPit->X, LSPit->Y, plane_) > 0.0) ? 1.0 : -1.0);
		}
		// do the refine
		image(LSPit->X, LSPit->Y, plane_) = value;
		
		LSPit++;
	}

	return bitsOut;
}

