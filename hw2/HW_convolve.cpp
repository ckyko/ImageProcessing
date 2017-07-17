// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_convolve
//
//
//
void
HW_convolve(ImagePtr I1, ImagePtr Ikernel, ImagePtr I2){
	// kernel dimensions
	int ww = Ikernel->width();
	int hh = Ikernel->height();

	// error checking: must use odd kernel dimensions
	if (!(ww % 2 && hh % 2)) {
		fprintf(stderr, "IP_convolve: kernel size must be odd\n");
		return;
	}

	int w = I1->width();
	int h = I1->height();
	IP_copyImageHeader(I1, I2);

	// clear offset; restore later
	int xoffset = I1->xoffset();
	int yoffset = I1->yoffset();
	I1->setXOffset(0);
	I1->setYOffset(0);

	// pad image to handle border problems
	int	 padnum[4];
	ImagePtr Isrc;
	padnum[0] = padnum[2] = ww / 2;		// left and right  padding
	padnum[1] = padnum[3] = hh / 2;		// top  and bottom padding
	IP_pad(I1, padnum, REPLICATE, Isrc);	// replicate border

	// restore offsets
	I1->setXOffset(xoffset);
	I1->setYOffset(yoffset);

	// cast kernel into array weight (of type float)
	ImagePtr Iweights;
	IP_castChannelsEq(Ikernel, FLOAT_TYPE, Iweights);
	ChannelPtr<float> wts = Iweights[0];

	ImagePtr I1f, I2f;
	if (I1->maxType() > UCHAR_TYPE) {
		I1f = IP_allocImage(w + ww - 1, h + hh - 1, FLOATCH_TYPE);
		I2f = IP_allocImage(w, h, FLOATCH_TYPE);
	}

	int	t;
	float	sum;
	ChannelPtr<uchar> p1, p2, in, out;
	ChannelPtr<float> f1, f2, wt;
	for (int ch = 0; IP_getChannel(Isrc, ch, p1, t); ch++) {
		IP_getChannel(I2, ch, p2, t);
		if (t == UCHAR_TYPE) {
			out = p2;
			for (int y = 0; y<h; y++) {	// visit rows
				for (int x = 0; x<w; x++) {	// slide window
					sum = 0;
					in = p1 + y*(w + ww - 1) + x;
					wt = wts;
					for (int i = 0; i<hh; i++) {	// convolution
						for (int j = 0; j<ww; j++)
							sum += (wt[j] * in[j]);
						in += (w + ww - 1);
						wt += ww;
					}
					*out++ = (int)(CLIP(sum, 0, MaxGray));
				}
			}
			continue;
		}
		IP_castChannel(Isrc, ch, I1f, 0, FLOAT_TYPE);
		f2 = I2f[0];
		for (int y = 0; y<h; y++) {		// visit rows
			for (int x = 0; x<w; x++) {		// slide window
				sum = 0;
				f1 = I1f[0];
				f1 += y*(w + ww - 1) + x;
				wt = wts;
				for (int i = 0; i<hh; i++) {	// convolution
					for (int j = 0; j<ww; j++)
						sum += (wt[j] * f1[j]);
					f1 += (w + ww - 1);
					wt += ww;
				}
				*f2++ = sum;
			}
		}
		IP_castChannel(I2f, 0, I2, ch, t);
	}
}
