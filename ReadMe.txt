========================================================================
    SPIHT CODEC project
========================================================================

copyright 2009-2010 Jan Maly
(honzamaly@gmail.com)

for the purposes of PhD thesis
"Modern Methods of Wavelet-Based Image Compression"
(at)
Brno University of Technology, Czech Republic
Faculty of Electrical Engineering and Telecommunications

Support:
Olivier Venard (ESIEE Paris)
Zdenek Prusa (BUT FEEC)

Special thanks to:
Pavel Rajmic (BUT FEEC)

=========================
1. PREFACE

Welcome to the SPIHT CODEC project. The intention of this utility is to demonstrate efficiency of proposed image compression solution based on discrete-time wavelet transform (DTWT) and the SPIHT (Set Partitioning In Hierarchical Trees) method of coefficient ordering and transmission. The goal is also to provide testing implementations of other reference solutions for color image compression, such as the CSPIHT.

=========================
2. PURPOSE

This utility is designed to be an easy to use Win32 console application. It has the ability to read and save 24-bit uncompressed BMP files, to encode/decode them using the selected technique and to store the resulting bistream as a file. It also computes a variety of properties, such as PSNR of the resulting image, elapsed time on given configuration, dynamic list counts and bits transmitted on various steps of the algorithm.

=========================
3. INSTALATION

The application consists of one small executable "codec.exe". No instalation is neccessary but the Visual Studio 2008 Runtime libraries, if your machine does not have Visual Studio 2008 installed. Microsoft Visual C++ 2008 Redistributable Package (x86) can be found here:
http://www.microsoft.com/downloads/details.aspx?FamilyID=9b2da534-3e03-4391-8a4d-074b9f2bc1bf&displaylang=en

Since 0.2, application takes use of Intel Threading Building Blocks developer library (so far only for objective time measurements). Library tbb.dll is included in the package and it's supposed to be in the same directory as the executable.

=========================
4. USAGE

The application was developed as a command-line tool, thus it is necessary to learn a few basic parameters of the executable.

Parameters are presented by parameter symbol (a minus sign "-" followed by letter, such as -a) and space-separated parameter value. An example of complete parameter can be "-i file1.bmp", which specifies file to be opened. Some parameters don't contain a value (flags such as -c). 

Parameter list:
-i file	: input image. Reads image (currently only 24-bit BMP files are supported) and prepares it for compression.
-o file	: output image. After decoding, output the result into 24-bit BMP file.
-b file	: [future] read / save bitstream. Reads bitstream file (.spi) and prepares it for decompression, saves BMP / Reads BMP file and does compression, saves .spi

(Basic working modes:
-i file -o file : performs coding and decoding with desired parameters. No bitstream is stored, only graphic information along with results of compression are outputted.
-i file -b file : performs coding with desired paramters and stores the resulting bitstream. No decoding done.
-b file -o file	: performs decoding and saves the resulting file. No coding done.)

-c		: CSPIHT used (default is BSPIHT)
-d		: DSPIHT used (default is BSPIHT)
-l		: levels of wavelet transform (1..x, will generate error if level too high for input image).
-S		: value of color level shift property (0..x), default is 0.
-B		: bits to compress / decompress. Specified exactly by number. NOTE: for decompression, if specified and less than bitstream size, the value overrides it (progressive decoding)
-p		: desired bpp (bits per pixel). Use this instead of -B.
-v		: desired variance depth. Defaults to 0.
-D		: print debug info
-E		: print extended info about compression
-T		: print timing info for profiling, measured by tbb::tick_count

NOTE: if no -B or -p is specified, application tries to do MAX_STEPS decoding (nearly lossless transformation).
NOTE: if no -l is specified, application assumes level=5.
Priority of algorithms using all flags (-c -d):
1. CSPIHT
2. DSPIHT

=========================
5. DEVELOPMENT NOTES

This application was created using C++ on a Visual Studio 2008 IDE. It has been programmed to extensively take advantage of the C++ Standard Template Library and uses most modern approaches in C++ development in order to minimize design errors and maximize code quality. Release version is optimized using Intel C++ Compiler v.11.

Should you encounter any bugs in the release versions, please notify me by email. Thanks in advance.

=========================
6. CHANGELOG


-- releases ---------------


-- development versions ---

v0.3
	- CHANGE: code refactoring using OOP
	- CHANGE: Improved speed of image matrixes handling
	- ADD: Processing with DSPIHT (Degraded Trees) single channel algorithm, use with flag -d
	- ADD: Color Level Shift property. Computation of variance possible by enabling "carry on" forward WT (Non-Deep Variance Computation).

v0.2
	- CHANGE: more effective CSPIHT root node tree is used [1].
	- ADD: Processing with base SPIHT algorithm [2]. Color computation by separation using Total Variance model [3]. Three bitstreams are processed independently.
	- ADD: Parameter processing with simple management
	- ADD: Now shows elapsed time for SPIHT processing (coding & decoding). Measured thread-safe using TBB tick_counters. Library tbb.dll included.
	- ADD: Compression rate in BPP controlled by the -p flag, followed by a floating point number
	- FIX: Assertion fault in class BitStream fixed
v0.1
	- initial implementation of CSPIHT




==========================
7. REFERENCES

[1] Kassim, A.A. and Wei Siong Lee - Embedded color image coding using SPIHT with partially linked spatial orientation trees, IEEE Transactions onCircuits and Systems for Video Technology, 2003, Vol. 13, No. 2, pp. 203-206

[2] Amir Said and William A. Pearlman, A new fast and efficient image codec based on set partitioning in hierarchical trees, IEEE Transactions on Circuits and Systems for Video Technology, 1996, Vol. 6, pp. 243-250

[3] Jan Maly and Olivier Venard and Zdenek Prusa, Simple Color Image Compression Method Using SPIHT with Degraded Trees, 2009 [unpublished]


