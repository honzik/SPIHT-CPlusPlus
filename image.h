// image class - loading, RGB2YUV transforms, saving
#ifndef IMAGE_H
#define IMAGE_H

// index transform of (0,1,2) RGB -> BMP representation
#define BMP_RPLANE 2
#define BMP_GPLANE 1
#define BMP_BPLANE 0

// specifies value of plane
enum planeVal { y=0, cB=1, cR=2 };

#include "general.h"
#include "settings.h"

// class holds together 3 matrices in an Image
// has methods for loading BMP, saving BMP
// has methods to perform RGB2YUV / YUV2RGB
// has assigment and initialization enabled
class Image {
	// storage space
	Matrix<wUnit> *image_;
	unsigned width_;
	unsigned height_;
	bool loaded_;

public:
	// SERVICE METHODS -------------------
	// implicit constructor, create memory
	Image();
	// copy constructor
	Image(const Image& copy);
	// assigment operator
	Image& operator= (const Image& src);
	// destructor
	~Image();

	// PROPERTIES ------------------------
	// get max value of whole plane
	wUnit getMax(unsigned plane) const;

	// detect if in the range X,Y,X+Size,Y+Size in the plane P, a value is present that is >= given value
	bool maxTest(wCoord X, wCoord Y, wCoord size, unsigned plane, wUnit value) const;

	// IMPORT / EXPORT (BMP)--------------
	bool loadBMP(const char *filename);
	bool saveBMP(const char *filename);

	// MODIFIERS -------------------------
	// round values towards nearest integer
	void roundValues();
	// -128..128 format
	void substract128();
	// 0..255 format
	void add128();
	// RGB to YCbCr without rounding
	void transformRGB2YCbCr();
	// YCbCr to RGB with rounding
	void transformYCbCr2RGB();
	// zero image
	// parameter: width / height
	void clear(unsigned width, unsigned height);

	// ACCESS TO VALUES ------------------
	// () operator overload - mutator
	inline wUnit& operator() (unsigned x, unsigned y, planeVal plane) {
		return image_[(unsigned) plane](x,y);
	}
	
	// () operator overload - inspector
	inline wUnit operator() (unsigned x, unsigned y, planeVal plane ) const {
		return image_[(unsigned) plane](x,y);
	}

	// get matrix (by ref)
	Matrix<wUnit>& getMatrix(planeVal p) const;

	// set matrix (by ref)
	// void setMatrix(unsigned i, Matrix<wUnit> mtx);

	// GLOBAL CHARACTERISTICS ------------- 
	// PSNR difference from other image (lumma)
	wUnit getLummaDifferencePSNR(Image &diff) const;
	// PSNR difference from other image (chroma)
	wUnit getChromaDifferencePSNR(Image &diff) const;
	
	// CODEC ALGORITHMS -------------------
	// get mean value of given set
	wUnit computeRangeMean(unsigned x, unsigned y, unsigned w, unsigned h, planeVal p);
	// get variance value of given set. 
	wUnit computeRangeVariance(unsigned x, unsigned y, unsigned w, unsigned h, planeVal p);
	// compute total variance up to given depth n.
	wUnit computeTotalVariance(Settings &sets, planeVal p);

	// PROPERTIES CHECKOUT ----------------
	// get image size w
	unsigned getWidth() const;

	// get image size h
	unsigned getHeight() const;

	// get image loaded status
	bool isLoaded() const;
};


// BMP images header structure
#pragma pack(1)
typedef struct                       
{
    unsigned short bfType;           /* Magic number for file */
    unsigned int   bfSize;           /* Size of file */
    unsigned short bfReserved1;      /* Reserved */
    unsigned short bfReserved2;      /* ... */
    unsigned int   bfOffBits;        /* Offset to bitmap data */
    unsigned int   biSize;           /* Size of info header */
    int            biWidth;          /* Width of image */
    int            biHeight;         /* Height of image */
    unsigned short biPlanes;         /* Number of color planes */
    unsigned short biBitCount;       /* Number of bits per pixel */
    unsigned int   biCompression;    /* Type of compression to use */
    unsigned int   biSizeImage;      /* Size of image data */
    int            biXPelsPerMeter;  /* X pixels per meter */
    int            biYPelsPerMeter;  /* Y pixels per meter */
    unsigned int   biClrUsed;        /* Number of colors used */
    unsigned int   biClrImportant;   /* Number of important colors */
} SHeader;

#pragma pack()
#define BF_TYPE 0x4D42             /* "MB" */


#endif


