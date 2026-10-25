#include "IP.h"
using namespace IP;

extern void HW_gammaCorrect(ImagePtr I1, double gamma, ImagePtr I2);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_errDiffusion:
//
// Apply error diffusion algorithm to image I1.
//
// This procedure produces a black-and-white dithered version of I1.
// Each pixel is visited and if it + any error that has been diffused to it
// is greater than the threshold, the output pixel is white, otherwise it is black.
// The difference between this new value of the pixel from what it used to be
// (somewhere in between black and white) is diffused to the surrounding pixel
// intensities using different weighting systems.
//
// Use Floyd-Steinberg     weights if method=0.
// Use Jarvis-Judice-Ninke weights if method=1.
//
// Use raster scan (left-to-right) if serpentine=0.
// Use serpentine order (alternating left-to-right and right-to-left) if serpentine=1.
// Serpentine scan prevents errors from always being diffused in the same direction.
//
// A circular buffer is used to pad the edges of the image.
// Since a pixel + its error can exceed the 255 limit of uchar, shorts are used.
//
// Apply gamma correction to I1 prior to error diffusion.
// Output is saved in I2.
//
void
HW_errDiffusion(ImagePtr I1, int method, bool serpentine, double gamma, ImagePtr I2)
{
	// gamma correct input image prior to dithering
	ImagePtr I3 = NEWIMAGE;
	HW_gammaCorrect(I1, gamma, I3);

	// copy image header (width, height) of input image I1 to output image I2
	IP_copyImageHeader(I1, I2);

	// init vars for width, height, and total number of pixels
	int w = I1->width ();
	int h = I1->height();

	// diffusion weights: kernel rows 0..2 (current row and two rows below),
	// columns -2..+2 relative to the current pixel. Entries at or behind the
	// current pixel on row 0 are zero since those pixels were already visited.
	int fs [3][5] = {			// Floyd-Steinberg (sum = 16)
		{0, 0, 0, 7, 0},
		{0, 3, 5, 1, 0},
		{0, 0, 0, 0, 0}
	};
	int jjn[3][5] = {			// Jarvis-Judice-Ninke (sum = 48)
		{0, 0, 0, 7, 5},
		{3, 5, 7, 5, 3},
		{1, 3, 5, 3, 1}
	};
	int (*wt)[5] = (method == 1) ? jjn : fs;
	int  norm    = (method == 1) ?  48 :  16;

	// 3-row circular buffer of shorts; each row is zero-padded by 2 pixels
	// on both sides so that errors may be diffused beyond the image borders
	const int pad = 2;
	int bw = w + 2*pad;
	short *buf[3];
	for(int k=0; k<3; k++) buf[k] = new short[bw];

	// declarations for image channel pointers and datatype
	ChannelPtr<uchar> p1, p2, in;
	int type;

	// visit all image channels and evaluate output image
	for(int ch=0; IP_getChannel(I3, ch, p1, type); ch++) {	// get input  pointer for channel ch
		IP_getChannel(I2, ch, p2, type);		// get output pointer for channel ch

		// load (cast to short and pad) input row y into buffer row k;
		// rows below the image are filled with zeros
		auto loadRow = [&](int y, int k) {
			short *b = buf[k];
			for(int x=0; x<bw; x++) b[x] = 0;
			if(y < h) {
				in = p1 + y*w;
				for(int x=0; x<w; x++) b[x+pad] = *in++;
			}
		};

		// prime circular buffer with first three rows
		for(int k=0; k<3; k++) loadRow(k, k);

		for(int y=0; y<h; y++) {
			// buffer rows holding current row and the two rows below it
			short *row[3] = { buf[y%3], buf[(y+1)%3], buf[(y+2)%3] };

			// scan direction: serpentine reverses odd rows
			int dir = (serpentine && (y & 1)) ? -1 : 1;
			int x0  = (dir == 1) ? 0 : w-1;
			uchar *out = &p2[y*w];

			for(int i=0, x=x0; i<w; i++, x+=dir) {
				// threshold current pixel (which includes diffused error):
				// white if greater than threshold, black otherwise
				int val = row[0][x+pad];
				int res = (val > MXGRAY/2) ? MaxGray : 0;
				out[x]  = res;

				// diffuse error to unvisited neighbors; mirror kernel
				// horizontally when scanning right-to-left
				int err  = val - res;
				int used = 0;
				for(int r=0; r<3; r++)
					for(int c=-2; c<=2; c++)
						if(wt[r][c+2] && !(r == 0 && c == 1)) {
							int share = err * wt[r][c+2] / norm;
							row[r][x + pad + c*dir] += share;
							used += share;
						}

				// next pixel in scan direction receives the remainder, so
				// no error is lost to integer truncation
				row[0][x + pad + dir] += err - used;
			}

			// current row is finished: reuse its buffer slot for row y+3
			loadRow(y+3, y%3);
		}
	}

	for(int k=0; k<3; k++) delete [] buf[k];
}
