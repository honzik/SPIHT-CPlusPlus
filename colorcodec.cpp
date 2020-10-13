// colorcodec implementation
#include "colorcodec.h"
#include <iostream>
#include <fstream>

void ColorCodec::computeBandSize(Settings &sets, Image &image, planeVal plane) {
	// compute max steps
	bandSizeW_ = image.getWidth();
	bandSizeH_ = image.getHeight();
	
	// CLS property
	unsigned i = sets.levels;
	if(!sets.cspihtFlag && sets.colorShift > 0 && plane > 0) {
		i += sets.colorShift;	
	}
	
	while(i > 0) {
		bandSizeW_ /= 2;
		bandSizeH_ /= 2;
		i--;
	}
	
	// check if levels match image sizes due to log2 cond's
	if(bandSizeW_ < 2 || bandSizeW_ % 2 != 0) {
		std::cout << "Codec error: WT transform level set too high for image width!" << std::endl;
		throw ExcWrongBandSize();
	}
	if(bandSizeH_ < 2 || bandSizeH_ % 2 != 0) {
		std::cout << "Codec error: WT transform level set too high for image height!" << std::endl;
		throw ExcWrongBandSize();
	}
}

double ColorCodec::getElapsedTime() const {
	return elapsedTime_;
}

// load / save wrappers
bool ColorCodec::save(const char *filename) const {
	return dt_.save(filename);
}
bool ColorCodec::load(const char *filename) {
	return dt_.load(filename);
}

// "late" constructor of data group, inits header and bs_ capacity
void ColorCodec::DataGroup::DataGroupInit(unsigned int ver, unsigned int streams, unsigned int imageX, unsigned int imageY) {
	// setup header
	hdr_.version = (unsigned char) ver;
	hdr_.streamCount = (unsigned char) streams;
	hdr_.bitsPerElem = (unsigned char) 8*sizeof(ColorCodec::DataGroup::bitElem);
	hdr_.width = (unsigned short) imageX;
	hdr_.height = (unsigned short) imageY;
	
	// invoke capacity in stream
	bs_.reserve(streams);	
}

// return width of bitstream image
unsigned ColorCodec::DataGroup::getWidth() const {
	return hdr_.width;
}
// return height of bitstream image
unsigned ColorCodec::DataGroup::getHeight() const {
	return hdr_.height;
}

// load of bitstream
bool ColorCodec::DataGroup::load(const char *filename) {
	std::ifstream file;

	// open file
	file.open(filename, std::ios::binary);
	if(!file.is_open()) {
		std::cout << "Unable to open file \"" << filename << "\"" << std::endl; 
		return false;
	}

	// read header
	file.read((char *)&hdr_, sizeof(hdr_));
	if(file.gcount() != sizeof(hdr_)) {
		std::cout << "Main SPI header incomplete!" << std::endl;
		return false;
	}
	
	if(hdr_.streamCount == 0) {
		std::cout << "StreamCount is zero in the given file!" << std::endl;
		throw ExcWrongBitStream();
	}

	if(hdr_.bitsPerElem != 8 * sizeof(bitElem)) {
		std::cout << "BitsPerElem property (" << hdr_.bitsPerElem << ") doesn't match this codec (" << 8 * sizeof(bitElem) << ")!" << std::endl;
		throw ExcWrongBitStream();
	}

	// delete all streams
	bs_.clear();		
	// reserve capacity in streams
	bs_.reserve(hdr_.streamCount);
	
	// for each stream in pool save sub-header and store vector
	// doint it the safe-way
	for(unsigned p=0; p < hdr_.streamCount; ++p) {
		// get header
		ColorCodec::DataGroup::BitStream::SubStreamHeader hd;
		// write the subheader
		file.read((char *) &hd, sizeof(hd));
		if(file.gcount() != sizeof(hd)) {
			std::cout << "Stream subheader[" << p << "] incomplete!" << std::endl;
			return false;
		}
		
		// create new stream
		bs_.push_back(ColorCodec::DataGroup::BitStream(hd.maxSteps, hd.totalBits, hd.level));
	
		// reserve capacity in stream, get stream address
		void * ptr = bs_[p].reserveStreamSpace(hd.elements);
				
		// fill up the stream
		file.read((char *) ptr, sizeof(bitElem) * hd.elements);
		if(file.gcount() != sizeof(bitElem) * hd.elements) {
			std::cout << "Stream[" << p << "] incomplete!" << std::endl;
			return false;	
		}
	}	
	
	file.close();
	std::cout << "Bitstream \"" << filename << "\" loaded... OK" << std::endl;
	return true;
}


// save of bitstream
bool ColorCodec::DataGroup::save(const char *filename) const {
	std::ofstream file;
	
	// open the file for saving
	file.open(filename, std::ios::binary);
	if(!file.is_open()) {
		std::cout << "Can't write to file \"" << filename << "\"" << std::endl; 
		return false;
	}
	
	// save header
	file.write((char *) &hdr_, sizeof(hdr_));
	
	// for each stream in pool save sub-header and store vector
	// doint it the safe-way
	for(unsigned p=0; p < hdr_.streamCount; ++p) {
		// get header
		ColorCodec::DataGroup::BitStream::SubStreamHeader hd = bs_[p].getSubStreamHeader();
		// write the header
		file.write((char *) &hd, sizeof(ColorCodec::DataGroup::BitStream::SubStreamHeader));
		// write the stream
		file.write((char *) bs_[p].getVectorAddress(), hd.elements * sizeof(ColorCodec::DataGroup::bitElem));
	}	
	
	file.close();
	std::cout << "Bitstream saved to file \"" << filename << "\"... OK" << std::endl;
	return true;	
}

// single bitstream constructor
ColorCodec::DataGroup::BitStream::BitStream(unsigned char mxStep, unsigned int totalB, unsigned char level) :
	maxSteps_(mxStep), totalBits_(totalB), elements_(1), bitPos_(0), elemPos_(0), bitNr_(0), level_(level), finished(false)
{
	stream_.clear();
	stream_.push_back(0);
}

// perform close private member
void ColorCodec::DataGroup::BitStream::performClose() {
	if(!finished) {
		totalBits_ = bitPos_;
		bitPos_ = 0;
		bitNr_ = 0;
		elements_ = stream_.size();
		elemPos_ = 0;
		finished = true;
	}
}

// get: gets bit from the bitstream. 
// returns:	0,1 - bit, -1 - error (bitstream not closed or final bitcount reached)
unsigned char ColorCodec::DataGroup::BitStream::get() {
	
	// over the final size - return -1
	if(bitPos_ >= totalBits_) {
		performClose();
		return -1;
	}
	
	// modulo ++ for bitNr
	if(bitNr_ == 8*sizeof(ColorCodec::DataGroup::bitElem)) {
		bitNr_ = 0;
		elemPos_++;
		// validity test
		if(elemPos_ == stream_.size()) {
			performClose();
			return -1;
		}
	}
	
	bitPos_++;

	// read bit value
	return (stream_[elemPos_] & (1 << bitNr_++)) ? 1 : 0;
}

// put: puts bit into the bitstream
// returns true if OK or false if bitstream closed
bool ColorCodec::DataGroup::BitStream::put(bool bit) {
	if(finished)
		return false;

	// over the final size - let's close the stream and zero all
	if(bitPos_ >= totalBits_) {
		performClose();
		return false;
	}

	// modulo ++ for bitNr
	if(bitNr_ == 8*sizeof(ColorCodec::DataGroup::bitElem)) {
		bitNr_ = 0;
		stream_.push_back(0);
	}

	// set N'th bit of last item in vector to 1
	if(bit) {
		// OR with vector element
		stream_.back() = stream_.back() | (1 << bitNr_); 
	}

	// update the BitNr
	bitNr_++;
	bitPos_++;

	return true;
}

// check against settings &ref
// IMPORTANT! function is called ONLY in decoding phase
// returns bits number, which is either totalBits_ or non-zero smaller bits
unsigned ColorCodec::DataGroup::BitStream::checkSettings(Settings &sets, unsigned bits) {
	bits = (bits < totalBits_ && bits > 0) ? bits : totalBits_;
	if(sets.levels != level_) {
		if(!(sets.colorShift > 0 && !sets.cspihtFlag && sets.colorShift + sets.levels == level_)) {
			std::cout << "BitStream does not match current level property!" << std::endl;
			throw ExcWrongBitStream();
		}
	}
	if(bits > totalBits_) {
		std::cout << "BitStream does not contain enough bits!" << std::endl;
		throw ExcWrongBitStream();
	}
	
	if(bits < totalBits_) {
		std::cout << "Only " << bits << " out of " << totalBits_ << " will be decoded." << std::endl;
		totalBits_ = bits;
	}
	
	return bits;
}

// max steps get
unsigned char ColorCodec::DataGroup::BitStream::getMaxSteps() const {
	return maxSteps_;
}

// get total bits
unsigned ColorCodec::DataGroup::BitStream::getTotalBits() const {
	return totalBits_;
}

ColorCodec::DataGroup::BitStream::SubStreamHeader ColorCodec::DataGroup::BitStream::getSubStreamHeader() const {
	SubStreamHeader hd;
	hd.elements = elements_;
	hd.level = level_;
	hd.maxSteps = maxSteps_;
	hd.totalBits = totalBits_;
	return hd;
}

// return vector address for further copy
// copy immediately after!
// NASTY :)
void * ColorCodec::DataGroup::BitStream::getVectorAddress() const {
	return (void *) &stream_[0];
}

// reserve space for incoming data & return stream pointer
// copy immediately after!
// NASTY :)
void * ColorCodec::DataGroup::BitStream::reserveStreamSpace(unsigned size) {
	stream_.assign( size, 0 );
	elements_ = size;
	finished = true;
	return (void *) &stream_[0];
}


// check if DataGroup ok with version & streams
// exception will be thrown if not
void ColorCodec::DataGroup::DataGroupCheck(unsigned ver, unsigned streams) {
	if(ver != hdr_.version) {
		std::cout << "DataGroup Error! Version does not match used algorithm!" << std::endl;
		throw ExcWrongDataGroup();
	}
	if(streams != hdr_.streamCount) {
		std::cout << "DataGroup Error! Bad stream count for used algorithm!" << std::endl;
		throw ExcWrongDataGroup();
	}
}

// width / height wrappers
unsigned ColorCodec::getImageW() const {
	return dt_.getWidth();
}
unsigned ColorCodec::getImageH() const {
	return dt_.getHeight();
}

