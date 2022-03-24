#include <xparameters.h>
#include "xil_io.h"
#include "sevenSeg_new.h"


static const int digit_segs[10] =  {
		0b0111111,	//0 //GNU C extension allows binary constants
		0b0000110,	//1
		0b1011011,  //2
		0b1001111,  //3
		0b1100110,  //4
		0b1101101,  //5
		0b1111101,  //6
		0b0000111,  //7
		0b1111111,  //8
		0b1101111,  //9
};

void sevenseg_draw_digit (int position, int value)
{
	int segs, segs_mask, digit_mask, combined_mask;

	segs  = digit_segs[value];
	segs_mask = 127^segs;
	digit_mask = 255 ^ (1<<position);
   	combined_mask = segs_mask | (digit_mask << 7);

   	Xil_Out32(XPAR_SEVENSEG_0_S00_AXI_BASEADDR, combined_mask);
}
