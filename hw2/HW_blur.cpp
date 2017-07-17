// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_blur
//
// 
//
void
HW_BLUR1D(ChannelPtr<uchar> in, int len, int stride, int kernel_width, ChannelPtr<uchar> out) {
	int buffer_size = len + ((kernel_width - 1 / 2) * 2);
	uchar * buffer = new uchar[buffer_size];
	uchar * temp = buffer;
	int half_w = (kernel_width / 2);
	int sum = 0;
	//if (kernel_width > len) return;

	for (int i = 0; i < half_w; i++){
		*temp = *in;
		temp++;
	}

	for (int i = 0; i < len; i++){
		*temp = *in;
		in += stride;
		temp++;
	}

	in -= stride;

	for (int i = 0; i < half_w; i++){
		*temp = *in;
		temp++;
	}

	for (int pixel = 0; pixel < len; pixel++){
		for (int j = 0; j < kernel_width; j++){
			sum += buffer[pixel + j];
		}
		sum = sum / kernel_width;
		*out = sum;
		out += stride;
		sum = 0;
	}

	delete[] buffer;

}

