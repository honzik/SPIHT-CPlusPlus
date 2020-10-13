#include "cspiht.h"
#include <cmath>
#include <iostream>
#include <iomanip>
#include "tbb/tick_count.h"

// CSpiht constructor
CSpiht::CSpiht(Image& im) : image(im) {
	elapsedTime_ = 0.0;
	// call dataGroupInit
	dt_.DataGroupInit(version, 1, image.getWidth(), image.getHeight());
}

// CSpiht encode
void CSpiht::encode(Settings &sets) {
	// compute bandsizes
	computeBandSize(sets, image, (planeVal) 0);
	
	if(sets.printExtended)
		EXTENDED = true;
	else
		EXTENDED = false;
	
	if(sets.printTiming)
		TIMING = true;
	else
		TIMING = false;

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
	elapsedTime_ = (t1-t0).seconds();

	if(TIMING)
		std::cout << std::fixed << std::setprecision(8) << "Elapsed time on init = " << (t1-t0).seconds() << std::endl;

	// init params & bs
	n_ = nMax_;
	currThr_ = pow(2.0, (wUnit) nMax_);
	dt_.bs_.clear();
	dt_.bs_.push_back(ColorCodec::DataGroup::BitStream(nMax_, sets.bits, sets.levels));
	
	// ref to bitstream: is now bs
	ColorCodec::DataGroup::BitStream& bs = dt_.bs_[0];
	
	if(EXTENDED) {
		std::cout << "-----------------------" << std::endl;
		std::cout << "CSPIHT encoder enabled." << std::endl;
	}
	
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
			if(EXTENDED) {
				std::cout << "CSPIHT encoding done. " << sets.bits << " bits (" << std::setprecision(1) << std::fixed
					  << (double) sets.bits/8.0 << "B) stored in bitstream." << std::endl;
				std::cout << "-----------------------" << std::endl;
			}
			break; 
		}

		n_--; currThr_ /= 2.0; 
	}
}

// CSpiht decode
void CSpiht::decode(Settings &sets, unsigned desiredBits) {
	// compute bandsizes
	computeBandSize(sets, image, (planeVal) 0);
	
	if(sets.printExtended)
		EXTENDED = true;
	else
		EXTENDED = false;
		
	if(sets.printTiming)
		TIMING = true;
	else
		TIMING = false;
	
	// basic condition
	dt_.DataGroupCheck(version, 1);
	
	// clear & init image
	image.clear(dt_.getWidth(), dt_.getHeight());
	
	// ref to bitstream: is now bs
	ColorCodec::DataGroup::BitStream& bs = dt_.bs_[0];
	// check if this bs is OK
	
	// return number of bits
	unsigned bits = bs.checkSettings(sets, desiredBits);
	
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
	elapsedTime_ = (t1-t0).seconds();

	if(TIMING)
		std::cout << std::fixed << std::setprecision(8) << "Elapsed time on init = " << (t1-t0).seconds() << std::endl;
	
	// init params & bs
	n_ = nMax_;
	decodingOver_ = false;
	currThr_ = pow(2.0, (wUnit) nMax_);
	halfThr_ = currThr_ / 2.0;	
	
	
	if(EXTENDED) {
		std::cout << "-----------------------" << std::endl;
		std::cout << "CSPIHT decoder enabled." << std::endl;
	}
	
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

		if(EXTENDED) {
			std::cout << ((decodingOver_)?"F":"S") << std::setw(2) << currStep <<  ", bits=" 
				  << std::setw(6) << sout << "sp + " << std::setw(6) << rout << "rp (" 
				  << std::setw(7) << std::setprecision(1) << std::fixed << (double) (sout+rout) / 8.0 << "B) | "
				  << "LIS: " << std::setw(5) << LIS_.size() <<  ", LIP: " << std::setw(5) << LIP_.size() 
				  << ", LSP: " << std::setw(5) << LSP_.size() << std::endl;
		}

		// possible ending - lossless
		if(n_ == 0) {
			bs.performClose();
		}

		// detect possible ending
		if(decodingOver_) {
			if(EXTENDED) {
				std::cout << "CSPIHT decoding done. " << bits << " bits (" << std::setprecision(1) << std::fixed
					  << (double) bits/8.0 << "B) stored in bitstream." << std::endl;
				std::cout << "-----------------------" << std::endl;
			}
			break; 
		}

		n_--; currThr_ /= 2.0; halfThr_ /= 2.0;
	}
}

// ----------- private methods
// init LIS members - put root nodes in
// init LIP members - put 
void CSpiht::initLists() {
	// init LIP & LIS
	for(wCoord j=0; j < bandSizeH_; ++j)
		for(wCoord i=0; i < bandSizeW_; ++i) {
			LIS_.push_back(XYPT(i,j,y,typeA));
			LIP_.push_back(XYP(i,j,y));
		}
	
}
// computes max val of image and steps number
unsigned CSpiht::computeSteps() {
	
	// get max from all planes
	wUnit maxY = image.getMax(0);
	wUnit maxCb = image.getMax(1);
	wUnit maxCr = image.getMax(2);
	wUnit maxResult = 0.0;
	
	// dumb comparison :)
	if(maxY > maxCb && maxY > maxCr) {
		maxResult = maxY;
	} else if(maxCb > maxCr) {
		maxResult = maxCb; 
	} else {
		maxResult = maxCr;
	}

	// compute the n_max
	return (unsigned) floor(log2(maxResult));
}

// coding: does a sorting pass, output enabled, returns number of bits outputted
unsigned CSpiht::sortingPassC(DataGroup::BitStream &bs) {
	unsigned bitsOut = 0;

	// part 1: LIP processing
	std::list<XYP>::iterator LIPit = LIP_.begin();

	while(LIPit != LIP_.end()) {
		// backup iterator: fetch current item into it, move to the next
		std::list<XYP>::iterator LIPcurr = LIPit++;
		// check for significance
		if(abs(image(LIPcurr->X, LIPcurr->Y, LIPcurr->P)) >= currThr_) {
			// output 1
			if(!bs.put(1)) return bitsOut; else bitsOut++;
			// output sign
			if(!bs.put(image(LIPcurr->X, LIPcurr->Y, LIPcurr->P) >= 0.0)) return bitsOut; else bitsOut++;
			// move into LSP
			LSP_.push_back(XYP(LIPcurr->X, LIPcurr->Y, LIPcurr->P));
			// delete from LIP
			LIP_.erase(LIPcurr);
		} else {
			// output 0
			if(!bs.put(0)) return bitsOut; else bitsOut++;
		}
	}

	// part 2: LIS processing
	std::list<XYPT>::iterator LISit = LIS_.begin();
	while(LISit != LIS_.end()) {
		// backup iterator: fetch current item into it, move to the next
		std::list<XYPT>::iterator LIScurr = LISit;

		// check significance
		if(checkSignificance(LIScurr->X, LIScurr->Y, LIScurr->P, (LIScurr->T == typeA) ? true : false)) {
			// output 1
			if(!bs.put(1)) return bitsOut; else bitsOut++;
			
			// init base coordinates
			wCoord baseX = LIScurr->X; wCoord baseY = LIScurr->Y;
			// flag for color root node speciality (actually occurs only in color planes)
			bool topLeftNode = false;
			
			// detect special cases
			if(baseX < bandSizeW_ && baseY < bandSizeH_) {
				// LLtop: top-left node
				if(baseY % 2 == 0 && baseX % 2 == 0) {
					topLeftNode = true;
				// LLtop: top-right node
				} else if(baseY % 2 == 0 && baseX % 2 != 0) {
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
	
			// save P and perform special treatment for "inter"-planar nodes
			planeVal P = LIScurr->P;
			if(topLeftNode)
				P = cB;

			// check four (eight - topLeftNode) descendants directly
			for(int i = 1; i < (topLeftNode?9:5); ++i) {
				// change base coordinates to match corner of the quadgroup
				if(i == 2 || i == 6) {
					baseX++;
				} else if(i == 3 || i == 7) {
					baseX--; baseY++;
				} else if(i == 4 || i == 8) {
					baseX++;
				} else if(i == 5) {
					// topLeftNode - decrease baseX; baseY; increase P
					baseX--; baseY--; P = cR;
				}

				// process typeA
				if(LIScurr->T == typeA) {
					// test for significance (single-element)
					if(abs(image(baseX, baseY, P)) >= currThr_) {
						// output 1
						if(!bs.put(1)) return bitsOut; else bitsOut++;
						// output sign
						if(!bs.put(image(baseX, baseY, P) >= 0.0)) return bitsOut; else bitsOut++;
						// move into LSP
						LSP_.push_back(XYP(baseX,baseY,P));
					} else {
						// output 0
						if(!bs.put(0)) return bitsOut; else bitsOut++;
						// move to LIP
						LIP_.push_back(XYP(baseX,baseY,P));
					}

				// process typeB
				} else {
					// exception!
					if(topLeftNode && (i == 1 || i == 5)) continue;
					// otherwise add node as LIS typeA entry (partitioning)
					LIS_.push_back(XYPT(baseX, baseY, P, typeA));
				}
			}

			// possible typeB entry creation
			if(LIScurr->T == typeA) {
				// check if image allows more descendants
				if(baseX*2 < (wCoord) image.getWidth() && baseY*2 < (wCoord) image.getHeight()) {
					// put into LIS as entry type B
					LIS_.push_back(XYPT(LIScurr->X, LIScurr->Y, LIScurr->P, typeB));
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
unsigned CSpiht::refinementPassC(DataGroup::BitStream &bs) {
	unsigned bitsOut = 0;
	// LSP processing	
	std::list<XYP>::iterator LSPit = LSP_.begin();

	// force last time threshold
	wUnit lastThr = pow(2.0, (double) nMax_ - n_ + 1);
	unsigned compare = (unsigned) pow(2.0 , (double) nMax_ + 2);

	while(LSPit != LSP_.end()) {
		unsigned value = (unsigned) floor( abs(image(LSPit->X, LSPit->Y, LSPit->P)) * lastThr );
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
bool CSpiht::checkSignificance(wCoord X, wCoord Y, planeVal P, bool startNow) {
	// define starting check size
	wCoord size = 2;
	// define descendants base coords
	wCoord baseX; wCoord baseY;

	if(X < bandSizeW_ && Y < bandSizeH_) {
		// inside LLtop: compute LLtop part root node coords
		baseX = (wCoord) floor((wUnit) X / 2.0) * 2;
		baseY = (wCoord) floor((wUnit) Y / 2.0) * 2;

		// determine correct descendants
		if(Y % 2 == 0) {
			if(X % 2 == 0) {
				// 1) top left - exception of root node of color plane!S! (CSPIHT version 0.2)
				
				// first check the 8 relatives, if startNow is on
				if(startNow && image.maxTest(baseX, baseY, size, cB, currThr_)) return true;
				if(startNow && image.maxTest(baseX, baseY, size, cR, currThr_)) return true;
				
				// now we have to make 6 calls to checkSignificance and gather the results
				// NOTE: very broad check!
				if(checkSignificance(baseX+1,	baseY,		cB, true)) return true;
				if(checkSignificance(baseX,		baseY+1,	cB, true)) return true;
				if(checkSignificance(baseX+1,	baseY+1,	cB, true)) return true;
				if(checkSignificance(baseX+1,	baseY,		cR, true)) return true;
				if(checkSignificance(baseX,		baseY+1,	cR, true)) return true;
				if(checkSignificance(baseX+1,	baseY+1,	cR, true)) return true;

				// can't go beyond this point
				return false;

			} else {
				// 2) top right
				baseX = baseX + bandSizeW_;
			}
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
		baseX = 2*X; baseY = 2*Y;
	}

	// loop to most possible depth
	do {
		// search for significance
		if(startNow) {
			if(image.maxTest(baseX, baseY, size, P, currThr_)) return true; 
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
unsigned CSpiht::sortingPassD(DataGroup::BitStream &bs) {
	unsigned bitsOut = 0;
	signed char getBit = 0;

	// part 1: LIP processing
	std::list<XYP>::iterator LIPit = LIP_.begin();
	while(LIPit != LIP_.end()) {
		// backup iterator: fetch current item into it, move to the next
		std::list<XYP>::iterator LIPcurr = LIPit++;

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
				image(LIPcurr->X, LIPcurr->Y, LIPcurr->P) = currThr_ + halfThr_;
			else
				image(LIPcurr->X, LIPcurr->Y, LIPcurr->P) = -1.0 * (currThr_ + halfThr_);

			// move into LSP
			LSP_.push_back(XYP(LIPcurr->X, LIPcurr->Y, LIPcurr->P));
			// delete from LIP
			LIP_.erase(LIPcurr);
		}
	}

	// part 2: LIS processing
	std::list<XYPT>::iterator LISit = LIS_.begin();
	while(LISit != LIS_.end()) {
		// backup iterator: fetch current item into it, move to the next
		std::list<XYPT>::iterator LIScurr = LISit;

		// read a bit
		if((getBit = bs.get()) == -1) { decodingOver_ = true; return bitsOut; }
		bitsOut++;
		// check significance
		if(getBit == 1) {
			// init base coordinates
			wCoord baseX = LIScurr->X; wCoord baseY = LIScurr->Y;
			// flag for color root node speciality (actually occurs only in color planes)
			bool topLeftNode = false;
			
			// detect special cases
			if(baseX < bandSizeW_ && baseY < bandSizeH_) {
				// LLtop: top-left node
				if(baseY % 2 == 0 && baseX % 2 == 0) {
					topLeftNode = true;
				// LLtop: top-right node
				} else if(baseY % 2 == 0 && baseX % 2 != 0) {
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

			// save P and perform special treatment for "inter"-planar nodes
			planeVal P = LIScurr->P;
			if(topLeftNode)
				P = cB;
	
			// check four descendants directly
			for(int i = 1; i < (topLeftNode?9:5); ++i) {
				// change base coordinates to match corner of the quadgroup
				if(i == 2 || i == 6) {
					baseX++;
				} else if(i == 3 || i == 7) {
					baseX--; baseY++;
				} else if(i == 4 || i == 8) {
					baseX++;
				} else if(i == 5) {
					// topLeftNode - decrease baseX; baseY; increase P
					baseX--; baseY--; P = cR;
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
							image(baseX, baseY, P) = currThr_ + halfThr_;
						else
							image(baseX, baseY, P) = -1.0 * (currThr_ + halfThr_);
						
						// move into LSP
						LSP_.push_back(XYP(baseX,baseY,P));
					} else {
						// move to LIP
						LIP_.push_back(XYP(baseX,baseY,P));
					}

				// process typeB
				} else {
					// exception!
					if(topLeftNode && (i == 1 || i == 5)) continue;
					// otherwise add node as LIS typeA entry (partitioning)
					LIS_.push_back(XYPT(baseX, baseY, P, typeA));
				}
			}

			// possible typeB entry creation
			if(LIScurr->T == typeA) {
				// check if image allows more descendants
				if(baseX*2 < (wCoord) image.getWidth() && baseY*2 < (wCoord) image.getHeight()) {
					// put into LIS as entry type B
					LIS_.push_back(XYPT(LIScurr->X, LIScurr->Y, LIScurr->P, typeB));
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
unsigned CSpiht::refinementPassD(DataGroup::BitStream &bs) {
	// exit upon finished reading
	if(decodingOver_)
		return 0;

	unsigned bitsOut = 0;
	signed char getBit = 0;
	// LSP processing iterator
	std::list<XYP>::iterator LSPit = LSP_.begin();

	// force last time threshold
	wUnit lastThr = pow(2.0, (double) n_ - 1);

	// limit for loop
	wUnit limit = pow(2.0, (double) n_ + 1);

	// read bits, "refine" pixels in image marked by LSP
	while(LSPit != LSP_.end()) {
		
		// prepare value
		wUnit value = image(LSPit->X, LSPit->Y, LSPit->P);
		if(abs(value) <= limit)
			break;

		// get a bit
		if((getBit = bs.get()) == -1) { decodingOver_ = true; return bitsOut; }
		bitsOut++;
		
		if(getBit == 1) {
			// positive add
			value = value + lastThr * ((image(LSPit->X, LSPit->Y, LSPit->P) > 0.0) ? 1.0 : -1.0); 
		} else {
			// negative add
			value = value - lastThr * ((image(LSPit->X, LSPit->Y, LSPit->P) > 0.0) ? 1.0 : -1.0);
		}
		// do the refine
		image(LSPit->X, LSPit->Y, LSPit->P) = value;
		
		LSPit++;
	}

	return bitsOut;
}
