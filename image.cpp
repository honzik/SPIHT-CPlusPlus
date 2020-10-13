#include "image.h"

#include <iostream>
#include <fstream>
#include <string.h>
#include <cmath>

// implicit constructor, create memory
Image::Image(): width_(0), height_(0), loaded_(0) {
	image_ = new Matrix<wUnit> [3];
}

// copy constructor
Image::Image(const Image& copy): width_(0), height_(0), loaded_(0) {
	image_ = new Matrix<wUnit> [3];
	if(copy.isLoaded()) {
		width_ = copy.getWidth();
		height_ = copy.getHeight();
		loaded_ = true;
		for(unsigned i=0; i<3; ++i)
			image_[i] = copy.getMatrix((planeVal) i);
	}
}

// assigment operator
Image& Image::operator= (const Image& src) {
	// detect case of equality
	if(this != &src) {
		// source loaded 
		if(src.isLoaded()) {
			// exists? free this memory and realloc
			if(loaded_) {
				delete []image_;
				image_ = new Matrix<wUnit> [3];
			}
			width_ = src.getWidth();
			height_ = src.getHeight();
			loaded_ = true;
			for(unsigned i=0; i<3; ++i)
				image_[i] = src.getMatrix((planeVal) i);
		} else {
			// delete, re-alloc, mark not loaded
			if(loaded_) {
				delete []image_;
				image_ = new Matrix<wUnit> [3];
				loaded_ = false;
			}
		}
	}
	return *this; 
}

// destructor
Image::~Image() {
	delete []image_;
}

// get matrix (by ref)
Matrix<wUnit>& Image::getMatrix(planeVal p) const {
	if(p>2) p=cR;
	return image_[(unsigned) p];
}

//// set matrix (by value)
//void Image::setMatrix(unsigned i, Matrix<wUnit> mtx) {
//	if(i>2) i=2;
//	image_[i] = mtx;
//}


// PROPERTIES CHECKOUT ----------------
// get image size w
unsigned Image::getWidth() const {
	return width_;
}

// get image size h
unsigned Image::getHeight() const {
	return height_;
}

// get image loaded status
bool Image::isLoaded() const {
	return loaded_;
}

// load BMP image
// so far accepts only 24bit uncompressed
// 8bit per channel, values 0...255 !
bool Image::loadBMP(const char *filename) {
	std::ifstream file;

	// open file
	file.open(filename, std::ios::binary);
	if(!file.is_open()) {
		std::cout << "Unable to open file \"" << filename << "\"" << std::endl; 
		return false;
	}

	// BMP head
	SHeader bmpHeader;

	// fetch info from header
	file.read((char *)&bmpHeader, 54);
	if(file.gcount() < 54 || bmpHeader.bfType != BF_TYPE) {
		std::cout << "BMP header fault" << std::endl; 
		file.close();
		return false;
	}
	
	int sizex, sizey, bpp=0;
	sizex = bmpHeader.biWidth;
	sizey = bmpHeader.biHeight;
	bpp = bmpHeader.biBitCount;

	if(!(sizex > 0 && sizey > 0 && bpp == 24)) {
		std::cout << "BMP has unusable properties (size or bpp)" << std::endl; 
		file.close();
		return false;
	}

	// alloc space for image contents
	char * buffer = new char[sizex * sizey * 3];
	// read the image
	file.read(buffer, sizex * sizey * 3);
	if(file.gcount() < sizex * sizey * 3) {
		std::cout << "BMP image in file either damaged or incomplete" << std::endl; 
		delete []buffer;
		file.close();
		return false;
	}

	// init planes in image structure
	clear(sizex, sizey);

	// fill up with values
	char * ptr = buffer;
	for(int j=0; j < sizey; ++j) {
		for(int i=0; i < sizex; ++i) {
			image_[BMP_RPLANE](i,sizey-j-1) = (wUnit) (unsigned char) *ptr++;
			image_[BMP_GPLANE](i,sizey-j-1) = (wUnit) (unsigned char) *ptr++;
			image_[BMP_BPLANE](i,sizey-j-1) = (wUnit) (unsigned char) *ptr++;
		}
	}

	std::cout << "File \"" << filename << "\" loaded... OK" << std::endl;

	delete []buffer;
	// close file
	file.close();
	return true;
}

// save BMP image
// very simple, 24-bit format, BGR layout, 54byte header
// 8bit per channel, values 0...255 !
bool Image::saveBMP(const char *filename) {
	if(!loaded_) {
		std::cout << "Can't save as BMP: No image defined" << std::endl;
		return false;
	}

	std::ofstream file;

	// open the file for saving
	file.open(filename, std::ios::binary);
	if(!file.is_open()) {
		std::cout << "Can't write to file \"" << filename << "\"" << std::endl; 
		return false;
	}
	
	// init structure
	SHeader bHead;
	bHead.bfType = BF_TYPE;
	bHead.bfSize = 54 + (3 * width_ * height_);
	bHead.bfReserved1 = 0;
	bHead.bfReserved2 = 0;
	bHead.bfOffBits = 54;
	bHead.biSize = 40;
	bHead.biWidth = width_;
	bHead.biHeight = height_;
	bHead.biPlanes = 1;
	bHead.biBitCount = 24;
	bHead.biCompression = 0;
	bHead.biSizeImage = (3 * width_ * height_);
	bHead.biXPelsPerMeter = 7200;
	bHead.biYPelsPerMeter = 7200;
	bHead.biClrUsed = 0;
	bHead.biClrImportant = 0;

	// save structure to file
	file.write((char *) &bHead, 54);

	// now prepare buffer of chars to write
	char * buffer = new char[3 * width_ * height_];
	
	// prepare the bitmap
	char * ptr = buffer;
	for(unsigned j=0; j < height_; ++j) {
		for(unsigned i=0; i < width_; ++i) {
			*(ptr++) = (char) (unsigned char) image_[BMP_RPLANE](i,height_-j-1);
			*(ptr++) = (char) (unsigned char) image_[BMP_GPLANE](i,height_-j-1);
			*(ptr++) = (char) (unsigned char) image_[BMP_BPLANE](i,height_-j-1);
		}
	}

	// save the bitmap
	file.write(buffer, 3 * width_ * height_);

	std::cout << "Image saved to file \"" << filename << "\"... OK" << std::endl;
	delete []buffer;
	file.close();
	return true;
}

// substract -128 from the values
void Image::substract128() {
	if(loaded_) {
		for(int p=0; p<3; ++p)
			for(unsigned j=0; j<height_; ++j)
				for(unsigned i=0; i<width_; ++i)
					image_[p](i,j) = image_[p](i,j) - 128.0;
	}
}


// add +128 to the values
void Image::add128() {
	if(loaded_) {
		for(int p=0; p<3; ++p)
			for(unsigned j=0; j<height_; ++j)
				for(unsigned i=0; i<width_; ++i)
					image_[p](i,j) = image_[p](i,j) + (wUnit) 128;
	}
}

// round image values towards integer
void Image::roundValues() {
	if(loaded_) {
		for(int p=0; p<3; ++p)
			for(unsigned j=0; j<height_; ++j)
				for(unsigned i=0; i<width_; ++i)
					image_[p](i,j) = round(image_[p](i,j));
	}
}

// transform RGB to YCbCr
// based on Rec 601-1 specs
void Image::transformRGB2YCbCr() {
	if(loaded_) {
		for(unsigned j=0; j<height_; ++j)
			for(unsigned i=0; i<width_; ++i) {
				// fetch values
				wUnit r = image_[0](i,j);
				wUnit g = image_[1](i,j);
				wUnit b = image_[2](i,j);
				
				// do transform, values taken from MATLAB rgb2ycbcr.m
				image_[0](i,j) =  16.0  + 0.256788235294118 * r   + 0.504129411764706 * g		+ 0.0979058823529412 * b;
				image_[1](i,j) = 128.0  - 0.148223529411765 * r   - 0.290992156862745 * g		+ 0.4392156862745100 * b;
				image_[2](i,j) = 128.0  + 0.439215686274510 * r	- 0.367788235294118 * g		- 0.0714274509803921 * b;
			}
	}
}



// transform YCbCr to RGB
// based on Rec 601-1 specs
void Image::transformYCbCr2RGB() {
	if(loaded_) {
		for(unsigned j=0; j<height_; ++j)
			for(unsigned i=0; i<width_; ++i) {
				// fetch values
				wUnit y =  image_[0](i,j);
				wUnit cB = image_[1](i,j);
				wUnit cR = image_[2](i,j);

				// do transform, values taken from MATLAB ycbcr2rgb.m
				image_[0](i,j)	=	round( 1.16438356164384 * y      + 3.01124397397633e-007 * cB	+ 1.596026887335700		* cR	- 222.921617109194);
				image_[1](i,j)	=	round( 1.16438356164384 * y      - 0.39176253994145      * cB   - 0.812968292162205		* cR	+ 135.575409522967);
				image_[2](i,j)	=	round( 1.16438356164384 * y      + 2.01723263955646		 * cB   + 3.05426174524847e-006	* cR	- 276.836305795032);
				
				// range checking (a must)
				for(unsigned p=0; p<3; ++p) {
					if(image_[p](i,j) > 255)
						image_[p](i,j) = 255.0;
					if(image_[p](i,j) < 0)
						image_[p](i,j) = 0.0;
				}
			}
	}
}

// get absolute maximum of the whole plane
wUnit Image::getMax(unsigned plane) const {
	if(plane > 2)
		return 0.0;

	wUnit maxVal = 0.0;
	for(unsigned j=0; j < image_[plane].getH(); ++j)
		for(unsigned i=0; i < image_[plane].getW(); ++i) 
			if(fabs(image_[plane](i,j)) > maxVal)
				maxVal = fabs(image_[plane](i,j));

	return maxVal;
}

// detect if in the range X,Y,X+Size,Y+Size in the plane P
// a value is present, that is >= given V
bool Image::maxTest(wCoord X, wCoord Y, wCoord size, unsigned plane, wUnit value) const {
	// check range(s)
	if(plane > 2)
		return false;
	if((unsigned) X+size > width_ || (unsigned) Y+size > height_) 
		return false;

	// drop true if higher or equal value detected
	for(unsigned j=Y; j < (unsigned) Y+size; ++j)
		for(unsigned i=X; i < (unsigned)  X+size; ++i) {
			wUnit val = fabs(image_[plane](i,j));
			if(val >= value)
				return true;
		}

	// else drop false
	return false;
}
// PSNR difference from other image... luminance (Y)
wUnit Image::getLummaDifferencePSNR(Image &diff) const {
	// init check
	if(diff.getWidth() != width_ && diff.getHeight() != height_) {
		return 0.0;
	}

	wUnit sum = 0.0;
	for(unsigned j=0; j < height_; ++j)
		for(unsigned i=0; i < width_; ++i)
			sum += (diff(i,j,y) - image_[0](i,j)) * (diff(i,j,y) - image_[0](i,j));

	sum = sqrt(sum / (width_ * height_));
	sum = 20.0 * log10(255.0 / sum) * 100.0;
	sum = round(sum);
	return sum/100.0;
}

// PSNR difference from other image... chrominance (Cb,Cr)
wUnit Image::getChromaDifferencePSNR(Image &diff) const {
	// init check
	if(diff.getWidth() != width_ && diff.getHeight() != height_) {
		return 0.0;
	}

	wUnit sum = 0.0;
	for(unsigned p=1; p<3; ++p)
		for(unsigned j=0; j < height_; ++j)
			for(unsigned i=0; i < width_; ++i)
				sum += (diff(i,j,(planeVal) p) - image_[p](i,j)) * (diff(i,j,(planeVal) p) - image_[p](i,j));

	sum = sqrt(sum / (2.0 * width_ * height_));
	sum = 20.0 * log10(255.0 / sum) * 100.0;
	sum = round(sum);
	return sum/100.0;
}

// init image to new dimensions
// width, height included
void Image::clear(unsigned width, unsigned height) {
	loaded_ = true;
	width_ = width;
	height_ = height;
	
	for(unsigned p=0; p<3; ++p) 
		image_[p].init(width_, height_);
}

// get mean value of given set
wUnit Image::computeRangeMean(unsigned x, unsigned y, unsigned w, unsigned h, planeVal p) {
	unsigned pixelsTotal = w * h;

	// get sum of all pixels
	wUnit sum = 0.0;
	for(unsigned j=y; j<y+h; ++j)
		for(unsigned i=x; i<x+w; ++i)
			sum += image_[p](i,j);

	// compute & return mean
	return sum / (wUnit) pixelsTotal;
}

// get variance value of given set. 
// It's a variance^2 value, defined by sigma^2 = 1/pixelsTotal * sum[(eachPixel-meanValue)^2]
wUnit Image::computeRangeVariance(unsigned x, unsigned y, unsigned w, unsigned h, planeVal p) {
	unsigned pixelsTotal = w * h;
	if(pixelsTotal == 0)
		return 0.0;
	
	// range checking
	if(x + w > width_ || y + h > height_)
		return 0.0;

	wUnit mean = computeRangeMean(x, y, w, h, p);
	wUnit variance = 0.0;

	for(unsigned j=y; j<y+h; ++j)
		for(unsigned i=x; i<x+w; ++i)
			variance += (image_[p](i,j) - mean) * (image_[p](i,j) - mean);

	return variance / (wUnit) pixelsTotal;
}

// compute total variance up to given depth n.
// It's a computation of total variance. 
// sigmaTot^2 = sigmaLL^2 + sum[i=1..n][ 4^(i-1) * (sigmaHHi^2 + sigmaLHi^2 + sigmaHLi^2) ]
// warning: to get latest bandsize, fetches flwt static members last..
wUnit Image::computeTotalVariance(Settings &sets, planeVal p) {

	unsigned w = image_[p].bandSizeW;
	unsigned h = image_[p].bandSizeH;

	if(w == 0 || h == 0)
		return 0.0;
	
	unsigned varDepth=sets.varianceDepth;

	if(p != y && sets.computeDeepVariance) {
		varDepth += sets.colorShift;
	}

	// first SUM element (sigmaLL^2)
	wUnit sum = computeRangeVariance(0, 0, w, h, p);

	// this can be done until condition is OK
	unsigned i = 0;
	while(w < width_ && h < height_ && i < varDepth) {
		wUnit tempSum = 0.0;
		
		// compute detail subbands of this level
		tempSum += computeRangeVariance(w, 0, w, h, p);
		tempSum += computeRangeVariance(0, h, w, h, p);
		tempSum += computeRangeVariance(w, h, w, h, p);
		
		// multiplier (4^L-i)
		tempSum *= (wUnit) pow((wUnit) 4, (wUnit) i);
		
		// add to sum
		sum += tempSum;

		// increase w,h,i
		w *= 2; h *= 2; i++;
	}
	
	//// add color level shift modifier for Y plane
	//if(sets.computeDeepVariance && sets.colorShift > 0 && p == y) {
	//    // color level shift mod 
	//	unsigned CLSmod = (unsigned) pow((float) 4.0, (float) sets.colorShift);
	//	sum *= CLSmod;
	//}	

	return sum;
}
