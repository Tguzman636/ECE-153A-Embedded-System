/*
 * sevenSeg_new.h
 *
 *  Created on: Oct 7, 2016
 *      Author: davidmc
 */


// New driver for the seven segment display. Haven't packaged this as a true driver yet, so you
// have to add it to your project directly for now.
//
// Exports one function, which lights up a single digit of the seven segment display. See lab
// handout for theory about this. Position 0 is rightmost, least significant, digit and position
// 7 is the leftmost, most significant digit. Digit is 0-9.
//
// If you called the seven segment display something other than "sevenseg_0" in the block diagram
// you'll get an error like:
//
//		"XPAR_SEVENSEG_0_S00_AXI_BASEADDR" undeclared (first used in this function)
//
// In this case open xparameters.h in your board support packaged and find the appropriate name
// based on what you called the block, and change the Xil_Out32 line in sevenSeg_new.c

#ifndef SRC_SEVENSEG_NEW_H_
#define SRC_SEVENSEG_NEW_H_


extern void sevenseg_draw_digit (int position, int value);



#endif /* SRC_SEVENSEG_NEW_H_ */
