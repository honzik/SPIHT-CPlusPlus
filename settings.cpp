// settings definitions
#include "settings.h"

#include <cstdlib>
#include <iostream>

// bailOut function
void Settings::bailOut(const char err[]) {
	std::cout << "Command line parameters: " << std::endl;
	std::cout << "  " << err << std::endl;
	exit(-2);
}

// checks whenever params are OK to go
// exits app when something wrong
void Settings::checkUsability() {
	// mode 1: read file, output file
	if(!(inputImage.empty() || outputImage.empty())) {
		mode = imageToImage;
	// mode 2: read file, save to bitstream
	} else if(!(inputImage.empty() || bitStreamFile.empty())) {
		mode = imageToBitstream;
	// mode 3: read bitstream, save to file
	} else if(!(outputImage.empty() || bitStreamFile.empty())) {
		mode = bitstreamToImage;
	// all other choices ==> error
	} else {
		bailOut("Unsupported mode of operation. Read ReadMe.txt and specify files correctly!");
	}	 
}

// class Settings constructor
Settings::Settings(int arc, char** arv) 
	: computeDeepVariance(COMPUTE_DEEP_VARIANCE), biasCR(BIAS_CR), biasCB(BIAS_CB) {
	
	// setting default values
	inputImage = std::string("");
	outputImage = std::string("");
	bitStreamFile = std::string("");

	cspihtFlag = false;
	dspihtFlag = false;
	levels	   = 3;
	colorShift = 0;
	varianceDepth = 0;
	bits	   = 2048;
	bpp		   = 0.0;
	mode	   = notDefined;
	
	// outputs
	printDebug = false;
	printTiming = false;
	printExtended = false;

	// parse the arguments
	if(arc > 1) {
		unsigned i=1;
		while(i < (unsigned) arc) {
			if(arv[i][0] == '-') {
				switch(arv[i][1]) {
					case	'i':
						if(++i < (unsigned) arc) {
							inputImage.assign(arv[i]);
						} else {
							bailOut("Input image file not specified.");
						}
						break;
					case	'o':
						if(++i < (unsigned) arc) {
							outputImage.assign(arv[i]);
						} else {
							bailOut("Output image file not specified.");
						}
						break;
					case	'b':
						if(++i < (unsigned) arc) {
							bitStreamFile.assign(arv[i]);
						} else {
							bailOut("Bitstream file not specified.");
						}
						break;
					case	'c':
						cspihtFlag = true;
						break;
					case	'D':
						printDebug = true;
						break;
					case	'E':
						printExtended = true;
						break;
					case	'T':
						printTiming = true;
						break;
					case	'd':
						dspihtFlag = true;
						break;
					case	'l':
						if(++i < (unsigned) arc) {
							levels = (unsigned) atoi(arv[i]);
							if(levels == 0) {
								bailOut("Levels must be positive and nonzero.");	
							}
						} else {
							bailOut("Levels number not specified.");
						}
						break;
					case	'S':
						if(++i < (unsigned) arc) {
							colorShift = (unsigned) atoi(arv[i]);
							if(colorShift < 0) {
								bailOut("Colorshift must be positive.");	
							}
						} else {
							bailOut("Colorshift number not specified.");
						}
						break;
					case	'v':
						if(++i < (unsigned) arc) {
							varianceDepth = (unsigned) atoi(arv[i]);
						} else {
							bailOut("Variance depth number not specified.");
						}
						break;
					case	'B':
						if(++i < (unsigned) arc) {
							bits = (unsigned) atoi(arv[i]);
						} else {
							bailOut("Bits number not specified.");
						}
						break;
					case	'p':
						if(++i < (unsigned) arc) {
							bpp = (float) atof(arv[i]);
							if(bpp <= 0) {
								bailOut("Bpp must be a floating-point number greater than zero (0.0).");
							}
						} else {
							bailOut("Bpp number not specified.");
						}
						break;	
				}
			} else {
				bailOut("Invalid specification. See ReadMe.txt for parameter layout.");
			}
			// increment
			i++;
		}
	}
	
	checkUsability();
}

