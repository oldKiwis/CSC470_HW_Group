#include <vector>
#include "IP.h"
using namespace IP;
using std::vector;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_convolve:
//
// Convolve I1 with filter kernel in Ikernel.
// Output is in I2.
//
// The kernel is a 2-D array of floats. The whole input channel is copied
// into a padded uchar buffer (pixel replication) so that the kernel never
// falls off the image. The kernel is not flipped. Results are rounded and
// clipped to [0,MaxGray].
//
void
HW_convolve(ImagePtr I1, ImagePtr Ikernel, ImagePtr I2)
{
	// copy image header (width, height) of input image I1 to output image I2
	IP_copyImageHeader(I1, I2);

	// init vars for width, height
	int w = I1->width ();
	int h = I1->height();

	// kernel dimensions and pointer to kernel values
	int kw = Ikernel->width ();
	int kh = Ikernel->height();
	int kx = kw / 2;		// horizontal padding on each side
	int ky = kh / 2;		// vertical   padding on each side
	int type;
	ChannelPtr<float> pk;
	IP_getChannel(Ikernel, 0, pk, type);
	float *kernel = &pk[0];

	// padded buffer for one input channel
	int bw = w + kw - 1;
	int bh = h + kh - 1;
	vector<uchar> buf(bw * bh);

	// declarations for image channel pointers
	ChannelPtr<uchar> p1, p2;

	// visit all image channels and evaluate output image
	for(int ch=0; IP_getChannel(I1, ch, p1, type); ch++) {	// get input  pointer for channel ch
		IP_getChannel(I2, ch, p2, type);		// get output pointer for channel ch

		// copy channel into padded buffer using pixel replication
		for(int y=0; y<bh; y++) {
			uchar *src = &p1[CLIP(y-ky, 0, h-1) * w];
			uchar *dst = &buf[y * bw];
			for(int x=0; x<bw; x++) dst[x] = src[CLIP(x-kx, 0, w-1)];
		}

		// multiply kernel with neighborhood of each pixel and sum
		for(int y=0; y<h; y++) {
			for(int x=0; x<w; x++) {
				float  sum = 0;
				float *k   = kernel;
				for(int r=0; r<kh; r++) {
					uchar *src = &buf[(y+r)*bw + x];
					for(int c=0; c<kw; c++) sum += *k++ * src[c];
				}
				*p2++ = CLIP(ROUND(sum), 0, MaxGray);
			}
		}
	}
}
