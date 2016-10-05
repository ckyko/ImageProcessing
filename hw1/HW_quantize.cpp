// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_quantize:
//
// Quantize I1 to specified number of levels. Apply dither if flag is set.
// Output is in I2.
//
// Written by: Kaiying Chen, Wailoon Chong, 2016
//
void
HW_quantize(ImagePtr I1, int levels, bool dither, ImagePtr I2)
{
	IP_copyImageHeader(I1, I2);
	int w = I1->width();
	int h = I1->height();
	int total = w * h;

	//compute the scale and bias
	double scale = (double)(MXGRAY / levels);
	double bias = scale / 2.0;

	// compute lut[]
	int i, lut[MXGRAY];
	for (i = 0; i<MXGRAY; i++)
		lut[i] = ((scale * (int)(i / scale)) + bias);

	int type;
	ChannelPtr<uchar> p1, p2, endd;
	if (!dither){
		for (int ch = 0; IP_getChannel(I1, ch, p1, type); ch++) {
			IP_getChannel(I2, ch, p2, type);
			for (endd = p1 + total; p1<endd;) *p2++ = lut[*p1++];
		}
	}
	else{
		int col, s, row, random_number, k;
		for (int ch = 0; IP_getChannel(I1, ch, p1, type); ch++) {
			IP_getChannel(I2, ch, p2, type);
			for (col = 0; col < h; ++col){
				s = col % 2 ? 1 : -1;
				for (row = 0; row < w; ++row, ++p1){
					random_number = (((double)rand()/RAND_MAX) * (int)bias) * s;
					s = s * -1;
					k = CLIP(*p1 + random_number, 0, MaxGray);
					*p2++ = lut[k];
				}
			}
		}
	}

}
