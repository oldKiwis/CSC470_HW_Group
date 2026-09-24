#include "IP.h"
using namespace IP;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_quantize:
//
// Quantize I1 to specified number of levels. Apply dither if flag is set.
// Output is in I2.
// The range [0,255] is split into levels uniform intervals of width
// scale = 256/levels; each input maps to its interval's midpoint,
// i.e., k*scale + bias, where bias = 128/levels.
// If dither is set, jitter in [0,bias] is alternately added to and
// subtracted from successive pixels prior to quantization.
//
void
HW_quantize(ImagePtr I1, int levels, bool dither, ImagePtr I2)
{
	// copy image header (width, height) of input image I1 to output image I2
	IP_copyImageHeader(I1, I2);

	// init vars for width, height, and total number of pixels
	int w = I1->width ();
	int h = I1->height();
	int total = w * h;

	// guard against invalid number of levels
	if(levels < 1)      levels = 1;
	if(levels > MXGRAY) levels = MXGRAY;

	// interval width and bias (half interval width)
	double scale = (double) MXGRAY / levels;
	double bias  = scale / 2;

	// init lookup table: map each gray level to midpoint of its interval
	int i, k, lut[MXGRAY];
	for(i=0; i<MXGRAY; ++i) {
		k = (int) (i / scale);			// interval index
		if(k > levels-1) k = levels-1;
		lut[i] = CLIP(ROUND(k*scale + bias), 0, MaxGray);
	}

	// declarations for image channel pointers and datatype
	ChannelPtr<uchar> p1, p2;
	int type;

	// visit all image channels and evaluate output image
	for(int ch=0; IP_getChannel(I1, ch, p1, type); ch++) {	// get input  pointer for channel ch
		IP_getChannel(I2, ch, p2, type);		// get output pointer for channel ch
		if(!dither) {
			for(i=0; i<total; i++) *p2++ = lut[*p1++];	// use lut[] to eval output
		} else {
			int sign = 1;
			for(i=0; i<total; i++) {
				// random jitter in [0,bias], alternately added and subtracted
				double jitter = (double) rand() / RAND_MAX * bias;
				int    val    = ROUND(*p1++ + sign*jitter);
				*p2++ = lut[CLIP(val, 0, MaxGray)];
				sign  = -sign;
			}
		}
	}
}
