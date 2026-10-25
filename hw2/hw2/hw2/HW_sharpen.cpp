#include "IP.h"
using namespace IP;

extern void HW_blur(ImagePtr I1, int filterW, int filterH, ImagePtr I2);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_sharpen:
//
// Sharpen image I1. Output is in I2.
// Blur I1 with a size x size box filter, then add the difference between
// I1 and its blurred version, scaled by factor, back to I1:
// output = CLIP(input + factor * (input - blurred), 0, MaxGray)
//
void
HW_sharpen(ImagePtr I1, int size, double factor, ImagePtr I2)
{
	// blur input image
	ImagePtr Iblur = NEWIMAGE;
	HW_blur(I1, size, size, Iblur);

	// copy image header (width, height) of input image I1 to output image I2
	IP_copyImageHeader(I1, I2);

	// init vars for width, height, and total number of pixels
	int w = I1->width ();
	int h = I1->height();
	int total = w * h;

	// declarations for image channel pointers and datatype
	ChannelPtr<uchar> p1, p2, pb;
	int type;

	// visit all image channels and evaluate output image
	for(int ch=0; IP_getChannel(I1, ch, p1, type); ch++) {	// get input  pointer for channel ch
		IP_getChannel(Iblur, ch, pb, type);		// get blurred pointer for channel ch
		IP_getChannel(I2,    ch, p2, type);		// get output  pointer for channel ch
		for(int i=0; i<total; i++) {
			int v = *p1++;
			int b = *pb++;
			*p2++ = CLIP(ROUND(v + factor*(v - b)), 0, MaxGray);
		}
	}
}
