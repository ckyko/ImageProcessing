// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_histoMatch:
//
// Apply histogram matching to I1. Output is in I2.
//
// Written by: ADD YOUR NAMES HERE, 2016
//
void
HW_histoMatch(ImagePtr I1, ImagePtr Ilut, ImagePtr I2)
{
	/**/
	IP_copyImageHeader(I1, I2);
	int w = I1->width();
	int h = I1->height();
	int total = w * h;

	//initialize all valueable we are going to use
	int  i, p, R;
	int left[MXGRAY], right[MXGRAY];
	int Hsum, h1[MXGRAY];
	double scale, Havg;

	int type;
	ChannelPtr<uchar> p1, p2, endd;
	for (int ch = 0; IP_getChannel(I1, ch, p1, type); ch++) {
		for (endd = p1 + total; p1<endd;) h1[*p1++]++;
	}

	// target histogram
	int *h2;

	ChannelPtr<int> temp;
	IP_getChannel(Ilut, 0, temp, type);
	h2 = (int *)&temp[0];
	//for (int z = 0; z < total; z++,h2++){
	//	qDebug() << *h2;
	//}

	/* normalize h2 to conform with dimensions of I1*/
	for (i = Havg = 0; i<MXGRAY; i++) Havg += h2[i];
	scale = (double)total / Havg;
	if (int(scale) != 1) for (i = 0; i<MXGRAY; i++) h2[i] = h2[i] * scale;

	R = 0;
	Hsum = 0;
	/* evaluate remapping of all input gray levels;
	Each input gray value maps to an interval of valid output values.
	The endpoints of the intervals are left[] and right[] */
	for (i = 0; i<MXGRAY; i++) {
		left[i] = R;								/* left end of interval */
		Hsum += h1[i];								/* cumulative value for interval */
		while (Hsum > h2[R] && R < MXGRAY - 1) {	/* compute width of interval */
			Hsum -= h2[R];							/* adjust Hsum as interval widens */
			R++;									/* update */
		}
		right[i] = R;								/* init right end of interval */
	}

	/* clear h1 and reuse it below */
	for (i = 0; i<MXGRAY; i++) h1[i] = 0;

	/* visit all input pixels */
	for (int ch = 0; IP_getChannel(I1, ch, p1, type); ch++) {
		IP_getChannel(I2, ch, p2, type);
		for (endd = p1 + total; p1<endd; p1++){
			p = left[*p1];
			if (h1[p] < h2[p])	/*mapping satisfies h2*/
				*p2++ = p;
			else *p2++ = p = left[*p1] = MIN(p + 1, right[*p1]);
			h1[p]++;

		}
	}
}
