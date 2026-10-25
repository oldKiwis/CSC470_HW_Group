#include <algorithm>
#include <vector>
#include "IP.h"
using namespace IP;
using std::vector;
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_median:
//
// Apply median filter of size sz x sz to I1.
// Clamp sz to 9.
// Output is in I2.
//
// The input is padded using pixel replication. A circular buffer of sz
// padded rows holds the rows needed to compute each output scanline.
// The sz*sz neighborhood values are sorted and the middle one is output.
//
void
HW_median(ImagePtr I1, int sz, ImagePtr I2)
{
	// copy input so that I1 and I2 may be the same image (the GUI does
	// this when applying multiple iterations)
	ImagePtr I3 = NEWIMAGE;
	IP_copyImage(I1, I3);

	// copy image header (width, height) of input image I3 to output image I2
	IP_copyImageHeader(I3, I2);

	// init vars for width, height
	int w = I3->width ();
	int h = I3->height();

	// clamp sz to [1,9] and force it to be odd
	if(sz < 1) sz = 1;
	if(sz > 9) sz = 9;
	if(sz % 2 == 0) sz--;
	int k  = sz / 2;		// padding on each side
	int bw = w + 2*k;		// padded row width

	// circular buffer of sz padded rows
	vector< vector<uchar> > buf(sz, vector<uchar>(bw));

	// neighborhood values and index of median
	vector<uchar> nbr(sz*sz);
	int mid = sz*sz / 2;

	// declarations for image channel pointers and datatype
	ChannelPtr<uchar> p1, p2;
	int type;

	// visit all image channels and evaluate output image
	for(int ch=0; IP_getChannel(I3, ch, p1, type); ch++) {	// get input  pointer for channel ch
		IP_getChannel(I2, ch, p2, type);		// get output pointer for channel ch

		// load padded row j (input row j-k, clamped to image) into its
		// circular buffer slot, replicating border pixels
		auto loadRow = [&](int j) {
			int y = CLIP(j-k, 0, h-1);
			uchar *src = &p1[y*w];
			uchar *dst = &buf[j % sz][0];
			for(int x=0; x<k; x++) dst[x]     = src[0];
			for(int x=0; x<w; x++) dst[x+k]   = src[x];
			for(int x=0; x<k; x++) dst[w+k+x] = src[w-1];
		};

		// prime circular buffer with first sz-1 padded rows
		for(int j=0; j<sz-1; j++) loadRow(j);

		for(int y=0; y<h; y++) {
			// add the next padded row; buffer now holds rows y..y+sz-1
			loadRow(y + sz-1);

			for(int x=0; x<w; x++) {
				// gather sz x sz neighborhood centered at (x,y)
				int n = 0;
				for(int r=0; r<sz; r++) {
					uchar *row = &buf[(y+r) % sz][x];
					for(int c=0; c<sz; c++) nbr[n++] = row[c];
				}

				// sort neighborhood and output its median
				std::sort(nbr.begin(), nbr.end());
				*p2++ = nbr[mid];
			}
		}
	}
}
