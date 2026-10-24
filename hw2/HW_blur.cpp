#include "IP.h"
using namespace IP;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_blur:
//
// Blur image I1 with a box filter (unweighted averaging).
// The filter has width filterW and height filterH.
// We force the kernel dimensions to be odd.
// Output is in I2.
//
void
HW_blur(ImagePtr I1, int filterW, int filterH, ImagePtr I2)
{

	// PUT YOUR CODE HERE

    IP_copyImageHeader(I1, I2);

    int w = I1->width ();
    int h = I1->height();

    if(filterW < 1) filterW = 1;
    if(filterH < 1) filterH = 1;
    if(filterW % 2 == 0) filterW++;
    if(filterH % 2 == 0) filterH++;

    int halfW = filterW / 2;
    int halfH = filterH / 2;

    uchar *line   = new uchar[w + 2*halfW];
    uchar *tmp    = new uchar[w * h];
    int   *colSum = new int[w];

    ChannelPtr<uchar> p1, p2;
    int type;

    for(int ch=0; IP_getChannel(I1, ch, p1, type); ch++) {
        IP_getChannel(I2, ch, p2, type);

        for(int y=0; y<h; y++) {
            for(int x=0; x<w; x++) line[halfW + x] = *p1++;
            for(int i=0; i<halfW; i++) {
                line[i]             = line[halfW];
                line[halfW + w + i] = line[halfW + w - 1];
            }

            int sum = 0;
            for(int i=0; i<filterW; i++) sum += line[i];
            for(int x=0; x<w; x++) {
                tmp[y*w + x] = (uchar) ((sum + filterW/2) / filterW);
                if(x < w-1) sum += line[x + filterW] - line[x];
            }
        }

        for(int x=0; x<w; x++) {
            colSum[x] = 0;
            for(int k=-halfH; k<=halfH; k++) {
                int r = (k < 0) ? 0 : ((k > h-1) ? h-1 : k);
                colSum[x] += tmp[r*w + x];
            }
        }

        for(int y=0; y<h; y++) {
            for(int x=0; x<w; x++) *p2++ = (uchar) ((colSum[x] + filterH/2) / filterH);

            int rin  = y + halfH + 1;
            int rout = y - halfH;
            if(rin  > h-1) rin  = h-1;
            if(rout < 0)   rout = 0;
            for(int x=0; x<w; x++) colSum[x] += tmp[rin*w + x] - tmp[rout*w + x];
        }
    }

    delete[] line;
    delete[] tmp;
    delete[] colSum;
}
