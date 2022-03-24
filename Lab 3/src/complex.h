/*
These functions return the product of two floating point complex numbers. 

Input
	re_a - the real part of the first number
	im_a - the imaginary part of the first number
	re_b - the real part of the first number
	im_b - the imaginary part of the first number
Returns
	mult_real returns the real part of the product. 
	mult_im returns the imaginary pary of the product.

*/

#ifndef COMPLEX_H
#define COMPLEX_H

float mult_real(float re_a, float im_a, float re_b, float im_b);

float mult_im(float re_a, float im_a, float re_b, float im_b);

#endif
