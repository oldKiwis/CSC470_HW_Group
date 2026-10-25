#include <vector>
#include "IP.h"
using namespace IP;
using std::vector;

void blur1D(uchar *, int, int, int, vector<int> &, uchar *);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_blur:
//
// Blur image I1 with a box filter (unweighted averaging).
// The filter has width filterW and height filterH.
// We force the kernel dimensions to be odd.
// Output is in I2.
//
// Separable implementation: first blur all rows of I1 into a temporary
// buffer, then blur all columns of that buffer into I2.
//
void HW_blur(ImagePtr I1, int filterW, int filterH, ImagePtr I2)
{
	// copy image header (width, height) of input image I1 to output image I2
	IP_copyImageHeader(I1, I2);

	// init vars for width, height, and total number of pixels
	int w = I1->width();
	int h = I1->height();
	int total = w * h;

	// force kernel dimensions to be odd and positive
	if (filterW < 1)
		filterW = 1;
	if (filterH < 1)
		filterH = 1;
	if (filterW % 2 == 0)
		filterW++;
	if (filterH % 2 == 0)
		filterH++;

	// temporary image holding the row-blurred result
	vector<uchar> tmp(total);

	// padded scanline buffer large enough for either pass
	vector<int> buf(MAX(w + filterW, h + filterH));

	// declarations for image channel pointers and datatype
	ChannelPtr<uchar> p1, p2;
	int type;

	// visit all image channels and evaluate output image
	for (int ch = 0; IP_getChannel(I1, ch, p1, type); ch++)
	{									 // get input  pointer for channel ch
		IP_getChannel(I2, ch, p2, type); // get output pointer for channel ch

		// pass 1: blur rows of I1 into tmp
		for (int y = 0; y < h; y++)
			blur1D(&p1[y * w], w, 1, filterW, buf, &tmp[y * w]);

		// pass 2: blur columns of tmp into I2
		for (int x = 0; x < w; x++)
			blur1D(&tmp[x], h, w, filterH, buf, &p2[x]);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// blur1D:
//
// Blur a scanline of len pixels (spaced stride apart) in src with a box
// filter of odd size sz. Output is written to dst with the same stride.
// The scanline is copied into buf with sz/2 pixels of padding on each
// side using pixel replication. A running sum is updated by adding the
// incoming pixel and subtracting the outgoing pixel as the window moves.
//
void blur1D(uchar *src, int len, int stride, int sz, vector<int> &buf, uchar *dst)
{
	int i, k = sz / 2;

	// copy scanline into padded buffer with pixel replication
	for (i = 0; i < k; i++)
		buf[i] = src[0];
	for (i = 0; i < len; i++)
		buf[i + k] = src[i * stride];
	for (i = 0; i < k; i++)
		buf[len + k + i] = src[(len - 1) * stride];

	// sum of the first window
	int sum = 0;
	for (i = 0; i < sz; i++)
		sum += buf[i];

	// slide window across scanline; output is rounded average
	for (i = 0; i < len; i++)
	{
		dst[i * stride] = (sum + sz / 2) / sz;
		if (i < len - 1)
			sum += buf[i + sz] - buf[i];
	}
}
