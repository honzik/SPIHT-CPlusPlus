// colorcodec: base class, color codec interface
#ifndef COLORCODEC_H
#define COLORCODEC_H

// print info about compression (lists, variances, steps...)
#define EXTENDED	printExtFlag_
// print timing info (useful for profiling)
#define TIMING		printTimeFlag_
// print debug info 
#define DEBUG		printDebugFlag_

#include <vector>
#include "image.h"
#include "settings.h"

// Base class.
// Declares encode(), decode(), load(), save() and getElapsedTime()
// Declares bitStream and support for whatever
class ColorCodec {
public:
	// BitStream declaration
	class DataGroup {
	public:
		typedef unsigned short bitElem;
		
		// header declaration, 7 bytes
		#pragma pack(1)
		struct Header {
			unsigned char version;
			unsigned char bitsPerElem;
			unsigned char streamCount;
			unsigned short width;
			unsigned short height;
		};
		#pragma pack()
		
		// bitstream class declaration
		class BitStream {
			// permanents
			unsigned char maxSteps_;
			unsigned totalBits_;
			unsigned char level_;
			unsigned elements_;
			std::vector<bitElem> stream_;
			
			// state values
			unsigned bitPos_;
			unsigned elemPos_;
			unsigned bitNr_;
					
		public:
			// subStream Header Datatype, 10 bytes
			#pragma pack(1)
			struct SubStreamHeader {
				unsigned char maxSteps;
				unsigned totalBits;
				unsigned char level;
				unsigned elements;	
			};
			#pragma pack()
			
			// constructor creates empty BitStream
			BitStream(unsigned char mxStep, unsigned totalB, unsigned char level);
			// get one bit and return 1/0/-1 (error)
			unsigned char get();
			// put one bit, return true (success), false (error)
			bool put(bool bit);
			// get max steps
			unsigned char getMaxSteps() const;
			// get total bits
			unsigned getTotalBits() const;
			// get substream header
			SubStreamHeader getSubStreamHeader() const;
			// gives address of first byte in the stream
			void * getVectorAddress() const;	
			// reserve space for incoming data & return stream pointer
			void * reserveStreamSpace(unsigned size);		
			// check bs against settings, returns either totalBits or specified bits (if smaller and non-zero)
			// also sets this value as new totalBits_
			unsigned checkSettings(Settings &sets, unsigned bits);
			// finished property
			bool finished;
			// "closer" member
			void performClose();
		};
	
		Header				   hdr_;	// header of the bitstream
		std::vector<BitStream> bs_;	// vector of streams

		// creates new DataGroup
		void DataGroupInit(unsigned ver, unsigned streams, unsigned imageX, unsigned imageY);
		// check if DataGroup ok with version & streams
		// exception will be thrown if not
		void DataGroupCheck(unsigned ver, unsigned streams);
		// save interface
		bool save(const char *filename) const;
		// load interface
		bool load(const char *filename);
		// return width of bitstream image
		unsigned getWidth() const;
		// return height of bitstream image
		unsigned getHeight() const;
	};
	// public base for encode 
	virtual void encode(Settings &sets) = 0;
	// public base for decode
	virtual void decode(Settings &sets, unsigned desiredBits=0) = 0;
	// getElapsedTime
	double getElapsedTime() const;
	// save and load wrappers
	bool save(const char *filename) const;
	bool load(const char *filename);
	// get width & height wrappers
	unsigned getImageW() const;
	unsigned getImageH() const;
	
protected:
	// output notifiers
	bool EXTENDED;
	bool DEBUG;
	bool TIMING;

	double elapsedTime_;	// time elapsed by last operation
	// main value
	DataGroup dt_;
	
	// bandsizes
	unsigned bandSizeW_;
	unsigned bandSizeH_;
	
	// exceptions
	class ExcWrongBandSize {};
	class ExcWrongBitStream {};
	class ExcWrongDataGroup {};
	class ExcWrongPlaneID {};
	
	// private bandsize computer
	// also checks whether everything OK
	// throws exception if err.
	void computeBandSize(Settings &sets, Image &image, planeVal plane);
};

#endif
