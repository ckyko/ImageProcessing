// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_histoMatch:
//
// Apply histogram matching to I1. Output is in I2.
//
// Written by: Kaiying Chen, Wailoon Chong, 2016
//
void
HW_histoMatch(ImagePtr I1, ImagePtr Ilut, ImagePtr I2)
{
	//initialize all valueable we are going to use
	int i, R;
	int left[MXGRAY], right[MXGRAY], index[MXGRAY], limit[MXGRAY], h_in[MXGRAY];
	int Hsum, Havg, *h_target;
	double scale, hmin, hmax;

	IP_copyImageHeader(I1, I2);
	int w = I1->width();
	int h = I1->height();
	int total = w * h;

	IP_embedRange(I1, 0, (double)MaxGray, I2);

	int type = 0;
	ChannelPtr<uchar> p, endd, temp;
	for (int ch = 0; IP_getChannel(I1, ch, p, type); ch++) {

		//get input image histogram
		IP_histogram(I1, ch, h_in, MXGRAY, hmin, hmax);

		// get target histogram
		IP_getChannel(Ilut, 0, temp, type);
		h_target = (int *)&temp[0];

		/* normalize h_target to conform with dimensions of I1*/
		for (i = Havg = 0; i < MXGRAY; i++) Havg += h_target[i];
		scale = (double)total / Havg;
		if (int(scale) != 1){
			for (i = 0; i < MXGRAY; i++){
				h_target[i] = h_target[i] * scale;
			}
		}

		R = 0;
		Hsum = 0;
		/* evaluate remapping of all input gray levels;
		Each input gray value maps to an interval of valid output values.
		The endpoints of the intervals are left[] and right[] */
		for (i = 0; i < MXGRAY; i++) {
			left[i] = index[i] = R;								/* left end of interval */
			limit[i] = h_target[R] - Hsum;                      /* set limmit */
			Hsum    += h_in[i];								    /* cumulative value for interval */
			while (Hsum > h_target[R] && R < MXGRAY - 1) {	    /* compute width of interval */
				Hsum -= h_target[R];							/* adjust Hsum as interval widens */
				R++;									        /* update */
			}
			right[i] = R;								        /* init right end of interval */
		}

		for (i = 0; i<MXGRAY; i++) h_in[i] = 0;
		IP_getChannel(I2, ch, p, type);
		for (endd = p + total; p<endd; p++){
			i = index[*p];
			if (i == left[*p]){
				if (limit[*p]-- <= 0){
					i = index[*p] = MIN(i + 1, MaxGray);
				}
				*p = i;
			}
			else if (i<right[*p]){
				if (h_in[i] < h_target[i]){
					*p = i;
				}
				else{
					*p = index[*p] = MIN(i + 1, MaxGray);
				}
			}
			else{
				*p = i;
			}
			h_in[i]++;
		}
	}
}
