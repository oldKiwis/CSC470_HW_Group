#include "IP.h"
using namespace IP;

void histoMatchApprox(ImagePtr, ImagePtr, ImagePtr);
static bool targetCDF(ImagePtr, int, int*);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_histoMatch:
//
// Apply histogram matching to I1. Output is in I2.
// The target histogram is given by the 256 entries in targetHisto.
// It is scaled so that its entries sum to the number of pixels in I1.
// Exact algorithm: input pixels are conceptually sorted by intensity and
// dealt out, in that order, to output bins filled to their target counts.
// A pixel's rank r among all pixels therefore determines its output bin:
// the bin b with C2[b] <= r < C2[b+1], where C2 is the target cumulative
// histogram. Since pixels of the same intensity are visited in raster
// order, each gets a distinct rank and the output histogram is exact.
//
void
HW_histoMatch(ImagePtr I1, ImagePtr targetHisto, bool approxAlg, ImagePtr I2)
{
	if(approxAlg) {
		histoMatchApprox(I1, targetHisto, I2);
		return;
	}

	// copy image header (width, height) of input image I1 to output image I2
	IP_copyImageHeader(I1, I2);

	// init vars for width, height, and total number of pixels
	int w = I1->width ();
	int h = I1->height();
	int total = w * h;

	// target cumulative histogram scaled to total: C2[b] = # target pixels < b
	int C2[MXGRAY+1];
	if(!targetCDF(targetHisto, total, C2)) return;

	// declarations for image channel pointers and datatype
	ChannelPtr<uchar> p1, p2, in;
	int type;

	int i, v, r;
	int h1[MXGRAY];		// input histogram
	int C1[MXGRAY];		// C1[i] = # input pixels with intensity < i
	int cnt[MXGRAY];	// # pixels of intensity i visited so far
	int bin[MXGRAY];	// current output bin for pixels of intensity i

	// visit all image channels and evaluate output image
	for(int ch=0; IP_getChannel(I1, ch, p1, type); ch++) {	// get input  pointer for channel ch
		IP_getChannel(I2, ch, p2, type);		// get output pointer for channel ch

		// compute input histogram
		for(i=0; i<MXGRAY; i++) h1[i] = 0;
		for(in=p1, i=0; i<total; i++) h1[*in++]++;

		// compute input cumulative histogram; init per-intensity state
		for(i=0, r=0; i<MXGRAY; i++) {
			C1[i]  = r;
			r     += h1[i];
			cnt[i] = 0;
			bin[i] = 0;
		}

		// assign each pixel to the output bin that contains its rank
		for(i=0; i<total; i++) {
			v = *p1++;
			r = C1[v] + cnt[v]++;		// rank of this pixel among all pixels
			while(bin[v] < MaxGray && r >= C2[bin[v]+1]) bin[v]++;
			*p2++ = bin[v];
		}
	}
}



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// histoMatchApprox:
//
// Apply approximate histogram matching (textbook version) to I1.
// Output is in I2.
// Both the input and target histograms are equalized via their
// normalized cumulative histograms T(i) and G(j). Each input intensity i
// is mapped to the smallest j for which G(j) >= T(i), i.e., G^-1(T(i)).
//
void
histoMatchApprox(ImagePtr I1, ImagePtr targetHisto, ImagePtr I2)
{
	// copy image header (width, height) of input image I1 to output image I2
	IP_copyImageHeader(I1, I2);

	// init vars for width, height, and total number of pixels
	int w = I1->width ();
	int h = I1->height();
	int total = w * h;

	// target cumulative histogram scaled to total: C2[b] = # target pixels < b
	int C2[MXGRAY+1];
	if(!targetCDF(targetHisto, total, C2)) return;

	// declarations for image channel pointers and datatype
	ChannelPtr<uchar> p1, p2, in;
	int type;

	int i, j, sum;
	int h1[MXGRAY], lut[MXGRAY];

	// visit all image channels and evaluate output image
	for(int ch=0; IP_getChannel(I1, ch, p1, type); ch++) {	// get input  pointer for channel ch
		IP_getChannel(I2, ch, p2, type);		// get output pointer for channel ch

		// compute input histogram
		for(i=0; i<MXGRAY; i++) h1[i] = 0;
		for(in=p1, i=0; i<total; i++) h1[*in++]++;

		// init lookup table: smallest j with G(j) >= T(i).
		// Both cumulative histograms are on the same scale (total),
		// so T(i) = # input pixels <= i and G(j) = C2[j+1].
		for(i=j=sum=0; i<MXGRAY; i++) {
			sum += h1[i];
			while(j < MaxGray && C2[j+1] < sum) j++;
			lut[i] = j;
		}

		for(i=0; i<total; i++) *p2++ = lut[*p1++];	// use lut[] to eval output
	}
}



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// targetCDF:
//
// Compute cumulative histogram C2 of the target histogram in targetHisto,
// scaled so that C2[MXGRAY] == total. C2[b] = # target pixels below b.
// Rounding the cumulative (rather than each entry) keeps the scaled
// entries C2[b+1]-C2[b] summing to exactly total.
// Return 1 for success, 0 if the target histogram is empty.
//
static bool
targetCDF(ImagePtr targetHisto, int total, int *C2)
{
	// get pointer to target histogram data
	int type;
	ChannelPtr<int> h2;
	IP_getChannel(targetHisto, 0, h2, type);

	// compute sum of target histogram entries
	double sum = 0;
	for(int i=0; i<MXGRAY; i++) sum += MAX(h2[i], 0);
	if(sum <= 0) return 0;

	// scale cumulative histogram to total number of pixels
	double scale = total / sum;
	double cum = 0;
	C2[0] = 0;
	for(int i=0; i<MXGRAY; i++) {
		cum += MAX(h2[i], 0);
		C2[i+1] = ROUND(cum * scale);
	}
	C2[MXGRAY] = total;
	return 1;
}
