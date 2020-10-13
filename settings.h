// settings class - parses command line parameters
#ifndef SETTINGS_H
#define SETTINGS_H

// if this is on, variance for color plane will be computed from depth level + CLS
// otherwise from level and after that, CLS-level forward transform is employed
#define COMPUTE_DEEP_VARIANCE false
// bias values for color plane processing
#define BIAS_CB 0.50
#define BIAS_CR 0.50

#include <string>

enum appMode {notDefined=0, imageToBitstream, bitstreamToImage, imageToImage};

// Settings:
// does fetch the command line
// converts all params into 
class Settings {
	// bail out function
	void bailOut(const char err[]);
	// checks whenever params are OK to go
	// exits app when something wrong
	void checkUsability();

public:
	// constructor
	Settings(int arc, char** arv);

	// public access properties
	std::string		inputImage;
	std::string		outputImage;
	std::string		bitStreamFile;
	bool		cspihtFlag;
	bool		dspihtFlag;
	unsigned	levels;
	unsigned	colorShift;
	unsigned	bits;
	unsigned	varianceDepth;
	float		bpp;
	
	// print info modifiers
	bool	printDebug;
	bool	printTiming;
	bool	printExtended;
	
	// non-direct fetch
	appMode		mode;
	// const init
	bool computeDeepVariance;
	double	biasCB;
	double	biasCR;
};

#endif