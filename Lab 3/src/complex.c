#include "complex.h"

/*
This function takes in two complex numbers as inputs. 
The form of the numbers are (re_a + im_a*j), (re_b + im_b*j).
It returns the real part of the multiplication of the inputs.
*/
float mult_real(float re_a, float im_a, float re_b, float im_b){
	return (re_a*re_b)-(im_a*im_b);
}


/*
This function takes in two complex numbers as inputs. 
The form of the numbers are (re_a + im_a*j), (re_b + im_b*j).
It returns the imaginary part of the multiplication of the inputs.
*/
float mult_im(float re_a, float im_a, float re_b, float im_b){
	return (re_a*im_b)+(re_b*im_a);
}
