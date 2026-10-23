#include "IP.h"
using namespace IP;

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

// PUT YOUR CODE HERE
    IP_copyImageHeader(I1, I2);

    int w = I1->width ();
    int h = I1->height();

    int lut[MXGRAY];
    for(int i=0; i<MXGRAY; i++) {
        double val = MaxGray * pow(i / (double) MaxGray, 1.0 / gamma);
        if(val < 0)       val = 0;
        if(val > MaxGray) val = MaxGray;
        lut[i] = (int) (val + 0.5);
    }

    const int fsDx [4]  = {1, -1, 0, 1};
    const int fsDy [4]  = {0,  1, 1, 1};
    const int fsWt [4]  = {7,  3, 5, 1};
    const int jjnDx[12] = {1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2};
    const int jjnDy[12] = {0, 0,  1,  1, 1, 1, 1,  2,  2, 2, 2, 2};
    const int jjnWt[12] = {7, 5,  3,  5, 7, 5, 3,  1,  3, 5, 3, 1};

    const int *dxs = (method == 0) ? fsDx : jjnDx;
    const int *dys = (method == 0) ? fsDy : jjnDy;
    const int *wts = (method == 0) ? fsWt : jjnWt;
    int nw    = (method == 0) ? 4  : 12;
    int denom = (method == 0) ? 16 : 48;

    int thr = MXGRAY / 2;
    const int pad = 2;
    int bw = w + 2*pad;

    short *buf[3];
    for(int r=0; r<3; r++) buf[r] = new short[bw];
    uchar *outRow = new uchar[w];

    ChannelPtr<uchar> p1, p2;
    int type;

    for(int ch=0; IP_getChannel(I1, ch, p1, type); ch++) {
        IP_getChannel(I2, ch, p2, type);

        for(int r=0; r<3; r++)
            for(int i=0; i<bw; i++) buf[r][i] = 0;

        for(int r=0; r<2 && r<h; r++)
            for(int x=0; x<w; x++) buf[r][x+pad] = lut[*p1++];

        for(int y=0; y<h; y++) {
            short *nxt = buf[(y+2) % 3];
            for(int i=0; i<bw; i++) nxt[i] = 0;
            if(y+2 < h)
                for(int x=0; x<w; x++) nxt[x+pad] = lut[*p1++];

            short *cur = buf[y % 3];
            int dir = (serpentine && (y & 1)) ? -1 : 1;
            int xs  = (dir == 1) ? 0 : w-1;

            for(int n=0; n<w; n++) {
                int x   = xs + dir*n;
                int val = cur[x+pad];
                int out = (val > thr) ? MaxGray : 0;
                outRow[x] = (uchar) out;

                int err  = val - out;
                int used = 0;
                for(int k=1; k<nw; k++) {
                    int share = err * wts[k] / denom;
                    used += share;
                    buf[(y + dys[k]) % 3][x + dir*dxs[k] + pad] += share;
                }
                buf[(y + dys[0]) % 3][x + dir*dxs[0] + pad] += err - used;
            }

            for(int x=0; x<w; x++) *p2++ = outRow[x];
        }
    }

    for(int r=0; r<3; r++) delete[] buf[r];
    delete[] outRow;

}
