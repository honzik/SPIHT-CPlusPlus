#include "flwt.h"
#include <iostream>
#include <cmath>

#define COEF_A	   -1.5861343420693648
#define COEF_B     -0.0529801185718856
#define COEF_C      0.8829110755411875 
#define COEF_D		0.4435068520511142
#define COEF_SCALE  1.1496043988602418

// forward row transform on WxH
void Flwt::rowTransformF(Matrix<wUnit>& source, unsigned W, unsigned H) {
	unsigned m = W;
	unsigned n = H;

	wUnit * tempbank = new wUnit[m];
	
	for(unsigned j = 0; j < n; ++j) {
		// PREDICT 1
		for(unsigned i = 1; i < m-2; i += 2)
			source(i,j) = source(i,j) + COEF_A * (source(i-1,j) + source(i+1,j));
		source(m-1,j) = source(m-1,j) + 2 * COEF_A * source(m-2,j);

		// UPDATE 1
		for(unsigned i = 2; i < m; i += 2)
			source(i,j) = source(i,j) + COEF_B * (source(i-1,j) + source(i+1,j));
		source(0,j) = source(0,j) + 2 * COEF_B * source(1,j);

		// PREDICT 2
		for(unsigned i = 1; i < m-2; i += 2)
			source(i,j) = source(i,j) + COEF_C * (source(i-1,j) + source(i+1,j));
		source(m-1,j) = source(m-1,j) + 2 * COEF_C * source(m-2,j);

		// UPDATE 2
		for(unsigned i = 2; i < m; i += 2)
			source(i,j) = source(i,j) + COEF_D * (source(i-1,j) + source(i+1,j));
		source(0,j) = source(0,j) + 2 * COEF_D * source(1,j);

		// REORDER
		for(unsigned i = 0; i < m; ++i)
			if(i % 2 == 0)
				tempbank[i/2] = source(i,j) * COEF_SCALE;
			else
				tempbank[m/2 + i/2] = source(i,j) / COEF_SCALE;

		// SAVE
		for(unsigned i = 0; i < m; ++i)
			source(i,j) = tempbank[i];
	}

	delete []tempbank;
}

// forward column transform on WxH
void Flwt::columnTransformF(Matrix<wUnit> &source, unsigned W, unsigned H) {
	unsigned m = W;
	unsigned n = H;

	wUnit * tempbank = new wUnit[n];
	
	for(unsigned i = 0; i < m; ++i) {
		// PREDICT 1
		for(unsigned j = 1; j < n-2; j += 2)
			source(i,j) = source(i,j) + COEF_A * (source(i,j-1) + source(i,j+1));
		source(i,n-1) = source(i,n-1) +  2 * COEF_A * source(i,n-2);

		// UPDATE 1
		for(unsigned j = 2; j < n; j += 2)
			source(i,j) = source(i,j) + COEF_B * (source(i,j-1) + source(i,j+1));
		source(i,0) = source(i,0) + 2 * COEF_B * source(i,1);

		// PREDICT 2
		for(unsigned j = 1; j < n-2; j += 2)
			source(i,j) = source(i,j) + COEF_C * (source(i,j-1) + source(i,j+1));
		source(i,n-1) = source(i,n-1) + 2 * COEF_C * source(i,n-2);

		// UPDATE 2
		for(unsigned j = 2; j < n; j += 2)
			source(i,j) = source(i,j) + COEF_D * (source(i,j-1) + source(i,j+1));
		source(i,0) = source(i,0) + 2 * COEF_D * source(i,1);

		// REORDER
		for(unsigned j = 0; j < n; ++j)
			if(j % 2 == 0) {
				tempbank[j/2] = source(i,j) * COEF_SCALE;
			} else {
				tempbank[n/2 + j/2] = source(i,j) / COEF_SCALE;
			}

		// SAVE
		for(unsigned j = 0; j < n; ++j)
			source(i,j) = tempbank[j];
	}

	delete []tempbank;
}

// inverse row transform on WxH
void Flwt::rowTransformI(Matrix<wUnit> &source, unsigned W, unsigned H) {
	unsigned m = W;
	unsigned n = H;

	wUnit * tempbank = new wUnit[m];
	
	for(unsigned j = 0; j < n; ++j) {
		// UNPACK
		for(unsigned i = 0; i < m/2; ++i) {
			tempbank[i*2] = source(i,j) / COEF_SCALE;
			tempbank[i*2+1] = source(i+m/2,j) * COEF_SCALE;
		}

		// STORE
		for(unsigned i = 0; i < m; ++i)
			source(i,j) = tempbank[i];


		// UPDATE 2
		for(unsigned i = 2; i < m; i += 2)
			source(i,j) = source(i,j) + (-1) * COEF_D * (source(i-1,j) + source(i+1,j));
		source(0,j) = source(0,j) + 2 * (-1) * COEF_D * source(1,j);

		// PREDICT 2
		for(unsigned i = 1; i < m-2; i += 2)
			source(i,j) = source(i,j) + (-1) * COEF_C * (source(i-1,j) + source(i+1,j));
		source(m-1,j) = source(m-1,j) + 2 * (-1) * COEF_C * source(m-2,j);

		// UPDATE 1
		for(unsigned i = 2; i < m; i += 2)
			source(i,j) = source(i,j) + (-1) * COEF_B * (source(i-1,j) + source(i+1,j));
		source(0,j) = source(0,j) + 2 * (-1) * COEF_B * source(1,j);

		// PREDICT 1
		for(unsigned i = 1; i < m-2; i += 2)
			source(i,j) = source(i,j) + (-1) * COEF_A * (source(i-1,j) + source(i+1,j));
		source(m-1,j) = source(m-1,j) + 2 * (-1) * COEF_A * source(m-2,j);
	}

	delete []tempbank;
}

// inverse column transform on WxH
void Flwt::columnTransformI(Matrix<wUnit> &source, unsigned W, unsigned H) {
	unsigned m = W;
	unsigned n = H;

	wUnit * tempbank = new wUnit[n];
	
	for(unsigned i = 0; i < m; ++i) {
		// UNPACK
		for(unsigned j = 0; j < n/2; ++j) {
			tempbank[j*2] = source(i,j) / COEF_SCALE;
			tempbank[j*2+1] = source(i,j+n/2) * COEF_SCALE;
		}

		// STORE
		for(unsigned j = 0; j < n; ++j)
			source(i,j) = tempbank[j];

		// UPDATE 2
		for(unsigned j = 2; j < n; j += 2)
			source(i,j) = source(i,j) + (-1) * COEF_D * (source(i,j-1) + source(i,j+1));
		source(i,0) = source(i,0) + 2 * (-1) * COEF_D * source(i,1);

		// PREDICT 2
		for(unsigned j = 1; j < n-2; j += 2)
			source(i,j) = source(i,j) + (-1) * COEF_C * (source(i,j-1) + source(i,j+1));
		source(i,n-1) = source(i,n-1) + 2 * (-1) * COEF_C * source(i,n-2);

		// UPDATE 1
		for(unsigned j = 2; j < n; j += 2)
			source(i,j) = source(i,j) + (-1) * COEF_B * (source(i,j-1) + source(i,j+1));
		source(i,0) = source(i,0) + 2 * (-1) * COEF_B * source(i,1);

		// PREDICT 1
		for(unsigned j = 1; j < n-2; j += 2)
			source(i,j) = source(i,j) + (-1) * COEF_A * (source(i,j-1) + source(i,j+1));
		source(i,n-1) = source(i,n-1) + 2 * (-1) * COEF_A * source(i,n-2);
	}

	delete []tempbank;
}

// forward transform wrapper (continuous)
// special: if output has already been a subject of transform, carry on from saved bandSizes, not WxH
void Flwt::forward(unsigned level, Matrix<wUnit>& output) {
	if(level > 0) {
		unsigned W = output.getW();
		unsigned H = output.getH();
		if(output.bandSizeW > 0 && output.bandSizeH > 0) {
			W = output.bandSizeW; H = output.bandSizeH; 
		} 
		for(unsigned d = 0; d < level; d++) {
			if(W%2 || H%2) {
				std::cout << std::endl << "FLWT::forward level setting wrong (too high)" << std::endl;
				break;
			}

			Flwt::columnTransformF(output, W, H);
			Flwt::rowTransformF(output, W, H);

			W = W/2;
			H = H/2;
		}

		output.bandSizeW = W;
		output.bandSizeH = H;

	} else {
		std::cout << std::endl << "FLWT::forward level setting wrong (0)" << std::endl;
	}
}

// inverse transfrom wrapper
void Flwt::inverse(unsigned level, Matrix<wUnit>& output) {
	
	if(level > 0) {
		// dimensions of the highest level
		unsigned W = output.getW();
		unsigned H = output.getH();
		unsigned i = level - 1;
	
		while(i > 0) {
			W /= 2;
			H /= 2;
			i--;
		}

		output.bandSizeW = W;
		output.bandSizeH = H;

		for(unsigned d = 0; d < level; d++) {
			if(W%2 || H%2) {
				std::cout << std::endl << "FLWT::inverse level setting wrong (too high)" << std::endl;
				break;
			}
			
			Flwt::rowTransformI(output, W, H);
			Flwt::columnTransformI(output, W, H);

			W = W*2;
			H = H*2;
		}
	} else {
		std::cout << std::endl << "FLWT::inverse level setting wrong (0)" << std::endl;
	}
}

