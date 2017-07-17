#define MAG(a, b)	(sqrt(a*a + b*b))
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// IP_correlation:
//
// Cross-correlation of image I1 with template I2.
// Correlation is performed on the first channel of I1 and I2 only.
//
// Multiresolution correlation is used if multires=1.
// This makes use of image pyramids for I1 and I2 and is significantly
// faster than direct correlation (multires=0).
//
// Return (dx,dy) offset of I2 for which there exists best match.
// Return the correlation number computed by method.
//
// The correlation methods are specified by mtd:
//	CROSS_CORR (cross correlation):
//		C(u,v) = sum of {T(x,y) * I(x-u,y-v)}
//			 --------------------------------
//			 sqrt{ sum of {I(x-u,y-v)^2}}
//	SSD (sum of squared differences):
//		C(u,v) = sum of {T(x,y)-I(x-u,y-v)}^2
//			 ------------------------------
//			 sqrt{ sum of {I(x-u,y-v)^2}}
//	CORR_COEFF (correlation coefficient):
//		C(u,v) = sum of {(T(x,y)-Tavg) * (I(x-u,y-v)-Iavg)}
//			 -----------------------------------------------------
//			 sqrt{sum{(T(x,y)-Tavg)^2} * sum{(I(x-u,y-v)-Iavg)^2}}
//	PHASE_CORR (phase correlation):
//				     {  FFT(T(x,y) * FFT*(I(x-u,y-v))  }
//		C(u,v) = inverse FFT { ------------------------------- }
//				     { |FFT(T(x,y) * FFT*(I(x-u,y-v))| }
//! \brief	Cross-correlation of image I1 with template I2.
//! \details	Correlation is performed on the first channel of I1 and I2 only.
//!
//!		Multiresolution correlation is used if multires=1.
//!		This makes use of image pyramids for I1 and I2 and is significantly
//!		faster than direct correlation (multires=0).
//!
//!		Return (dx,dy) offset of I2 for which there exists best match.
//!
//!		The correlation methods are specified by mtd:
//!
//!		CROSS_CORR (cross correlation):
//!		<pre>
//!		C(u,v) = sum of {T(x,y) * I(x-u,y-v)}
//!			 --------------------------------
//!			 sqrt{ sum of {I(x-u,y-v)^2}}
//!		</pre>
//!
//!		SSD (sum of squared differences):
//!		<pre>
//!		C(u,v) = sum of {T(x,y)-I(x-u,y-v)}^2
//!			 ------------------------------
//!			 sqrt{ sum of {I(x-u,y-v)^2}}
//!		</pre>
//!
//!		CORR_COEFF (correlation coefficient):
//!		<pre>
//!		C(u,v) = sum of {(T(x,y)-Tavg) * (I(x-u,y-v)-Iavg)}
//!			 -----------------------------------------------------
//!			 sqrt{sum{(T(x,y)-Tavg)^2} * sum{(I(x-u,y-v)-Iavg)^2}}
//!		</pre>
//!
//!		PHASE_CORR (phase correlation):
//!		<pre>
//!				     {  FFT(T(x,y) * FFT*(I(x-u,y-v))  }
//!		C(u,v) = inverse FFT { ------------------------------- }
//!				     { |FFT(T(x,y) * FFT*(I(x-u,y-v))| }
//!		</pre>
//! \return	The correlation number computed by method.
//
float
HW_correlation(ImagePtr I1, ImagePtr I2, int mtd, int multires, int &xx, int &yy)
{
	// init vars to suppress compiler warnings
	int	  dx = 0;
	int	  dy = 0;
	int   lowres = 0;
	float	mag  = 0;
	float	corr = 0;
	double	xsz, ysz;

	// image dimensions
	int w = I1->width ();
	int h = I1->height();

	// template dimensions
	int ww = I2->width ();
	int hh = I2->height();

	// error checking: size of image I1 must be >= than template I2
	if(!(ww<=w && hh<=h)) {
		fprintf(stderr, "Correlation: image is smaller than template\n");
		return 0.;
	}

	// cast image into buffer of type float
	ImagePtr II1;
	if(I1->channelType(0) != FLOAT_TYPE) {
		II1 = IP_allocImage(I1->width(), I1->height(), FLOATCH_TYPE);
		IP_castChannel(I1, 0, II1, 0, FLOAT_TYPE);
	} else	II1 = I1;

	// cast template into buffer of type float
	ImagePtr II2;
	if(I2->channelType(0) != FLOAT_TYPE) {
		II2 = IP_allocImage(I2->width(), I2->height(), FLOATCH_TYPE);
		IP_castChannel(I2, 0, II2, 0, FLOAT_TYPE);
	} else	II2 = I2;

	// create image and template pyramids with original images at base;
	// if no multiresolution is used, pyramids consist of only one level.
	int mxlevel;
	ImagePtr pyramid1[8], pyramid2[8];
	pyramid1[0] = II1;		// base: original image
	pyramid2[0] = II2;		// base: original template
	if(multires) {
		switch(mtd) {
		case CROSS_CORR:
		case SSD:
			lowres = 64;
			break;
		}
	} else	mxlevel = 0;

	// init search window
	int x1 = 0;
	int y1 = 0;
	int x2 = (w-ww)>>mxlevel;
	int y2 = (h-hh)>>mxlevel;

	// declarations
	int		  total;
	float		  sum1, sum2, avg, tmpl_pow;
	ChannelPtr<float> image, templ;
	ImagePtr	  Iblur, Ifft1, Ifft2;

	// multiresolution correlation: use results of lower-res correlation
	// (at the top of the pyramid) to narrow the search in the higher-res
	// correlation (towards the base of the pyramid).
	for(int n=mxlevel; n>=0; n--) {
	    // init vars based on pyramid at level n
	    w  = pyramid1[n]->width(); h  = pyramid1[n]->height();
	    ww = pyramid2[n]->width(); hh = pyramid2[n]->height();

	    // pointers to image and template data
	    ChannelPtr<float> p1 = pyramid1[n][0];	// image    ptr
	    ChannelPtr<float> p2 = pyramid2[n][0];	// template ptr

	    // init min and max
	    float min = 10000000.;
	    float max = 0.;

	    switch(mtd) {
	    case CROSS_CORR:				// cross correlation
		for(int y=y1; y<=y2; y++) {		// visit rows
		    for(int x=x1; x<=x2; x++) {		// slide window
			sum1  = sum2 = 0;
			image = p1 + y*w + x;
			templ = p2;
			for(int i=0; i<hh; i++) {	// convolution
		   		for(int j=0; j<ww; j++) {
					sum1 += (templ[j] * image[j]);
					sum2 += (image[j] * image[j]);
				}
				image += w;
				templ += ww;
			}
			if(sum2 == 0) continue;

			corr = sum1 / sqrt(sum2);
			if(corr > max) {
		   		max = corr;
		   		dx  = x;
		   		dy  = y;
			}
		    }
		}

		// update search window or normalize final correlation value
		if(n) {		// set search window for next pyramid level
			x1 = MAX(0,   2*dx - n);
			y1 = MAX(0,   2*dy - n);
			x2 = MIN(2*w, 2*dx + n);
			y2 = MIN(2*h, 2*dy + n);
		} else {	// normalize correlation value at final level
			tmpl_pow = 0;
			total	 = ww * hh;
			for(int i=0; i<total; i++)
				tmpl_pow += (p2[i] * p2[i]);
			corr =	max / sqrt(tmpl_pow);
		}
		break;

	    case SSD:				// sum of squared differences
		float diff;
		for(int y=y1; y<=y2; y++) {		// visit rows
		    for(int x=x1; x<=x2; x++) {		// slide window
			sum1  = sum2 = 0;
			image = p1 + y*w + x;
			templ = p2;
			for(int i=0; i<hh; i++) {	// convolution
		   		for(int j=0; j<ww; j++) {
					diff  = templ[j] - image[j];
					sum1 += (diff * diff);
					sum2 += (image[j] * image[j]);
				}
				image += w;
				templ += ww;
			}
			if(sum2 == 0) continue;

			corr = sum1 / sqrt(sum2);
			if(corr < min) {
		   		min = corr;
		   		dx  = x;
		   		dy  = y;
			}
		     }
		}

		// update search window or normalize final correlation value
		if(n) {		// set search window for next pyramid level
			x1 = MAX(0,   2*dx - n);
			y1 = MAX(0,   2*dy - n);
			x2 = MIN(2*w, 2*dx + n);
			y2 = MIN(2*h, 2*dy + n);
		} else {	// normalize correlation value at final level
			total = ww * hh;
			float	tmpl_pow = 0;
			for(int i=0; i<total; i++)
				tmpl_pow += (p2[i] * p2[i]);
			corr =	min / sqrt(tmpl_pow);
		}
		break;

	    default:
		fprintf(stderr, "Correlation: Bad mtd %d\n", mtd);
		return 0.;
	    }
	}

	xx = dx;
	yy = dy;
	return corr;
}
