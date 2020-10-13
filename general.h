// general functions

#ifndef GENERAL_H
#define GENERAL_H

#include <string.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>

// unit of pixel values
typedef double wUnit;
// unit of coordinate values
typedef unsigned short wCoord;
// general algorithm enum
enum typeVal { typeA, typeB }; 

// bitstream versioning
#define VER_CSPIHT 0x0A
#define VER_BSPIHT 0x0B

// template for matrix
// general 2D matrix template definition
// exception handling
template <class Type> class Matrix {
	// privates
	unsigned w_;
	unsigned h_;
	Type *map_;

public:	
	// exceptions
	class ExOutOfRange {};
	class ExNotDefined {};

	// empty map init constructor
	Matrix()
		: w_(0), h_(0), map_(0), bandSizeW(0), bandSizeH(0) {}

	// map init constructor to size
	Matrix(unsigned w, unsigned h)
		: w_(w), h_(h), map_(0), bandSizeW(0), bandSizeH(0) {
			init(w, h);
	}

	// copy constructor
	Matrix(const Matrix& copy): map_(0), w_(0), h_(0), bandSizeW(0), bandSizeH(0) {
		w_ = copy.getW();
		h_ = copy.getH();
		bandSizeW = copy.bandSizeW;
		bandSizeH = copy.bandSizeH;

		if(w_ == 0 || h_ == 0) {
			map_ = 0;
		} else {
			init(w_, h_);
			for(unsigned i=0; i<h_; ++i)
				memcpy((void *) &map_[i * w_], (void *) copy.getLine(i), sizeof(Type) * w_); 
		}
	}

	// assigment operator 
	Matrix& operator= (const Matrix& src) {
		// case of equality
		if(this != &src) {
			w_ = src.getW();
			h_ = src.getH();
			bandSizeW = src.bandSizeW;
			bandSizeH = src.bandSizeH;
		
			if(w_ == 0 || h_ == 0) {
				map_ = 0;
			} else {
				init(w_, h_);
				for(unsigned i=0; i<h_; ++i)
					memcpy((void *) &map_[i * w_], (void *) src.getLine(i), sizeof(Type) * w_);
			}
		}
		return *this; 
	}

	// () operator overload - mutator
	inline Type& operator() (unsigned x, unsigned y) {
		if(!map_)
			throw ExNotDefined();
		if(x >= w_ || y >= h_)
			throw ExOutOfRange();
		return map_[y*w_ + x];
	}

	// () operator overload - inspector
    inline Type operator() (unsigned x, unsigned y) const {
		if(!map_)
			throw ExNotDefined();
		if(x >= w_ || y >= h_)
			throw ExOutOfRange();
		return map_[y*w_ + x];
	}


	// init handler
	void init(unsigned w, unsigned h) {
		if(map_)
			delete []map_;
		if(w == 0 || h == 0) {
			return;
		}
		w_ = w;
		h_ = h;
		bandSizeW = 0; bandSizeH = 0;
		map_ = new Type[w * h];
		// delete all to zero
		memset((void *) map_, 0, sizeof(Type) * (w * h)); 
	}

	// free handler (for pairing)
	void free() {
		if(map_)
			delete []map_;
		w_ = 0;
		h_ = 0;
	}

	// get width
	unsigned getW() const {
		return w_;
	}

	// get height
	unsigned getH() const {
		return h_;
	}

	// return address of dedicated line
	Type* getLine(unsigned y) const {
		if(y >= h_)
			throw ExOutOfRange();
		return &map_[y * w_];
	}

	// copy whole matrix (ins) into matrix at position x,y of width and height w,h
	bool copyMatrix(unsigned x, unsigned y, unsigned w, unsigned h, Matrix ins) {
		// check if wide enough
		if(x + w > getW() || y + h > getH())		
			return false;
		// copy using lines
		for(unsigned i = 0; i < h; ++i) 
			memcpy((void *) ((Type *) (getLine(i + y) + x)), (void *) ins.getLine(i), sizeof(Type) * w);
		return true;
	}

	// export matrix to HTML table
	bool exportMatrix(const char *filename) {
		std::ofstream file;

		file.open(filename);
		if(!file.is_open()) {
			std::cout << "Can't write to file \"" << filename << "\"" << std::endl; 
			return false;
		}

		file << "<table><tr><th>.</th>" << std::endl;
		for(unsigned i=0; i<w_; ++i)
			file << "<th>" << i << "</th>";
		
		file << "</tr>" << std::endl;

		for(unsigned j=0; j<h_; ++j) {
			file << "<tr><th>" << j << "</th>";
			for(unsigned i=0; i<w_; ++i)
				file << "<td>" << std::setprecision(3) << map_[w_*j + i] << "</td>";
			file << "</tr>" << std::endl;
		}

		file << "</table>" << std::endl;
		file.close();
		std::cout << "Matrix exported to file named \"" << filename << "\"" << std::endl;

		return true;
	}

	// destructor
	~Matrix() {
		free();
	}

	unsigned bandSizeW;
	unsigned bandSizeH;
};

// general function prototypes - definitions in .cpp
wUnit log2(wUnit x);
wUnit round(wUnit x);


#endif
