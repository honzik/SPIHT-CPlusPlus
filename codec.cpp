// codec.cpp : main C++ file

#include <conio.h>
#include <iostream>
#include <vector>

#include "settings.h"
#include "image.h"
#include "flwt.h"
#include "cspiht.h"
#include "bspiht.h"
#include "dspiht.h"

int main(int argc, char **argv)
{
	// global times
	double timeEncoding = 0.0;
	double timeDecoding = 0.0;

	ColorCodec * codec = 0;

	// parse parameters
	Settings S(argc, argv);

	Image RGB, backup;

	unsigned level = S.levels;
	
	try {
	
		if(S.mode == imageToImage || S.mode == imageToBitstream) {
			if(RGB.loadBMP(S.inputImage.c_str())) {
				
				RGB.transformRGB2YCbCr();
				
				if(S.mode == imageToImage) {
					backup = RGB;
				}
				
				RGB.substract128();
			
				// forward WT
				for(unsigned p = 0; p < 3; p ++) {
					Matrix<wUnit>& plane = RGB.getMatrix((planeVal) p);
					// temporarily down
					if(S.computeDeepVariance && p > 0 && S.colorShift > 0 && !S.cspihtFlag) {
						if(S.printExtended)
							std::cout << "Performing " << level+S.colorShift << "-level forward WT on plane " << p << " (colorShifted +" << S.colorShift <<")...";
						Flwt::forward(level+S.colorShift, plane);
					} else {
						if(S.printExtended)
							std::cout << "Performing " << level << "-level forward WT on plane " << p << "...";
						Flwt::forward(level, plane);
					}
					if(S.printExtended)
						std::cout << "OK" << std::endl;
					//RGB.setMatrix(p, plane);
				}
				
				// bpp conversion
				if(S.bpp > 0.0) {
					S.bits = (unsigned) ceil(S.bpp * RGB.getWidth() * RGB.getHeight() * 3.0);
					std::cout << "Desired BPP=" << std::setprecision(2) << S.bpp << " means " << S.bits << "bits (" << std::setprecision(1) << std::fixed 
							  <<  S.bits/8.0 << "B) for a " << RGB.getWidth() << "x" << RGB.getHeight() << " image." << std::endl;
				}

				if(S.cspihtFlag)
					codec = new CSpiht(RGB);
				else if(S.dspihtFlag)
					codec = new DSpiht(RGB);
				else
					codec = new BSpiht(RGB);
				
				// well, isn't this nice :-)
				codec->encode(S);
				timeEncoding = codec->getElapsedTime();
				
				// if save enabled, save
				if(S.mode == imageToBitstream)
					if(!codec->save(S.bitStreamFile.c_str())) {
						delete codec;
						exit(-1);
					}
					
			}
		}

		// here will be BS loading
		
		if(S.printExtended)
			std::cout << std::endl << std::endl;

		// decoding part
		if(S.mode == imageToImage || S.mode == bitstreamToImage) {
			
			// perform decode SPIHT
			if(codec == 0) {
				if(S.cspihtFlag)
					codec = new CSpiht(RGB);
				else if(S.dspihtFlag)
					codec = new DSpiht(RGB);
				else
					codec = new BSpiht(RGB);
			} 
			
			if(S.mode == bitstreamToImage) {
				codec->load(S.bitStreamFile.c_str());
			}
			
			// TODO: customized decoding bit count (aux parameter to decode)
			if(S.bpp > 0.0 && S.mode == bitstreamToImage) {
				S.bits = (unsigned) ceil(S.bpp * codec->getImageW() * codec->getImageH() * 3.0);
				std::cout << "Desired BPP=" << std::setprecision(2) << S.bpp << " means " << S.bits << "bits (" << std::setprecision(1) << std::fixed 
						  <<  S.bits/8.0 << "B) for a " << codec->getImageW() << "x" << codec->getImageH() << " image." << std::endl;
			}
			
			
			codec->decode(S, S.bits);	
			timeDecoding = codec->getElapsedTime();

			if(S.printExtended)
				std::cout << std::endl;

			// perform inverse WT
			for(unsigned p = 0; p < 3; p ++) {
				Matrix<wUnit>& plane = RGB.getMatrix((planeVal) p);
				if(p > 0 && S.colorShift > 0 && !S.cspihtFlag) {
					if(S.printExtended)
						std::cout << "Performing " << level+S.colorShift << "-level inverse WT on plane " << p << " (colorShifted +" << S.colorShift <<")...";
					Flwt::inverse(level+S.colorShift, plane);
				} else {
					if(S.printExtended)
						std::cout << "Performing " << level << "-level inverse WT on plane " << p << "...";
					Flwt::inverse(level, plane);
				}
				if(S.printExtended)
					std::cout << "OK" << std::endl;
				//RGB.setMatrix(p, plane);
			}
			
            RGB.add128();

			// compute stuff
			if(backup.getWidth() == RGB.getWidth() && backup.getHeight() == RGB.getHeight()) {
				std::cout << std::endl;
				std::cout << "PSNR difference Y:     " << std::setprecision(2) << RGB.getLummaDifferencePSNR(backup) << "dB" << std::endl;	
				std::cout << "PSNR difference cB,cR: " << std::setprecision(2) << RGB.getChromaDifferencePSNR(backup) << "dB" << std::endl;	
				std::cout << std::endl;
			}

			if(timeEncoding > 0.0 && S.printTiming)
				std::cout << "SPIHT CODING time elapsed   (total): " << std::setprecision(8) << timeEncoding << "s" <<  std::endl;

			if(timeDecoding > 0.0 && S.printTiming)
				std::cout << "SPIHT DECODING time elapsed (total): " << std::setprecision(8) << timeDecoding << "s" <<  std::endl;

			// save image
			if(S.printExtended)
				std::cout << std::endl;
			
			RGB.transformYCbCr2RGB();
			RGB.saveBMP(S.outputImage.c_str());

		}
	}
	
	catch(...) {
		std::cout << "Exception occured. Program is now being terminated..." << std::endl;
	}

	std::cout << "press any key..." << std::endl;
	_getch();
	
	if(codec)
		delete codec;

	return 0;
}

