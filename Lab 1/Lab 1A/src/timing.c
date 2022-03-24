/*
 *timing.c: simple starter application for lab 1A and 1B
 *
 */

#include <stdio.h>		// Used for printf()
#include <stdlib.h>		// Used for rand()
#include "xparameters.h"	// Contains hardware addresses and bit masks
#include "xil_cache.h"		// Cache Drivers
#include "xintc.h"		// Interrupt Drivers
#include "xtmrctr.h"		// Timer Drivers
#include "xtmrctr_l.h" 		// Low-level timer drivers
#include "xil_printf.h" 	// Used for xil_printf()
#include "extra.h" 		// Provides a source of bus contention
#include "xgpio.h" 		// LED driver, used for General purpose I/i

#define NUMBER_OF_TRIALS 1000
#define NUMBER_OF_BINS 15
#define BUFFER_SIZE (1024*1024)
unsigned int buffer[BUFFER_SIZE]; //buffer for read/write operations to the DDR memory

/*
 * The following constant is used to determine which channel of the GPIO is
 * used for the LED if there are 2 channels supported.
 */
#define LED_CHANNEL 1

void histogram(void); // This function creates a histogram for the measured data

/*
 * The following are declared globally so they are zeroed and so they are
 * easily accessible from a debugger
 */
XGpio Gpio; /* The Instance of the GPIO Driver used for LED 0 */

/*
 * This globally declared array stores the
 * number of clock cycles for all the trials.
 * With global declaration, it is stored in the data segment of the
 * memory. Declaring this large array locally may cause a stack overflow.
 */
int numClockCycles[NUMBER_OF_TRIALS];

//Stores the number of samples in each bin
int histData[NUMBER_OF_BINS];


//BaseAddr points to the base (byte) address of the DDR2 Memory
u8 * BaseAddr = (u8 *) XPAR_MIG7SERIES_0_BASEADDR;


int main()
{
	Xil_ICacheInvalidate();
	Xil_ICacheEnable();
	Xil_DCacheInvalidate();
	Xil_DCacheEnable();
	print("---Entering main---\n\r");
	int i = 0;
	int sum_of_cycles = 0;
	int time_avg = 0;
	int event_avg = 0;
	int number_of_events = 40;
	int timer_val_before; //Used to store the timer value before executing the operation being timed
	u32 Addr;
	volatile unsigned int Data;


	// Extra Method contains an interrupt routine which is set to go off at timed intervals
    	extra_method();


 	//TIMER RESET CODE
		//Turn off the timer
		XTmrCtr_SetControlStatusReg(XPAR_TMRCTR_0_BASEADDR, 1, 0);
		//Put a zero in the load register
		XTmrCtr_SetLoadReg(XPAR_TMRCTR_0_BASEADDR, 1, 0);
		//Copy the load register into the counter register
		XTmrCtr_SetControlStatusReg(XPAR_TMRCTR_0_BASEADDR, 1, XTC_CSR_LOAD_MASK);
 		//Enable (start) the timer
 		XTmrCtr_SetControlStatusReg(XPAR_TMRCTR_0_BASEADDR, 1,	XTC_CSR_ENABLE_TMR_MASK);
 		//END TIMER RESET CODE

 	//INITIALIZATION FOR AXI GPIO LED PORT
 		XGpio_Initialize(&Gpio, XPAR_AXI_GPIO_LED_DEVICE_ID);
    for( i=0; i < NUMBER_OF_TRIALS; i++) {

    	Addr = rand()%BUFFER_SIZE; //Will be used to access a random buffer index
    	int int1 = rand();
		int int2 = rand();
    	float float1 = (float)rand()/(float)(RAND_MAX) * 5;
		float float2 = (float)rand()/(float)(RAND_MAX) * 5;
		float float_p = (float)rand()/(float)(RAND_MAX) * 5;
		XGpio_DiscreteWrite(&Gpio, LED_CHANNEL, 0x0);

    	timer_val_before = XTmrCtr_GetTimerCounterReg(XPAR_TMRCTR_0_BASEADDR, 1); //Store the timer value before executing the operation being timed

 		// Enter the line of Code to time.

    		//Data = int1+int2;
    		//float Data = float1+float2;
    		//XGpio_DiscreteWrite(&Gpio, LED_CHANNEL, 0x1); //Turns on one LED
    		//Data = buffer[Addr];
    		//printf("%f\n",float_p);
    		//xil_printf("tonyguzman\n");

 		numClockCycles[i] = XTmrCtr_GetTimerCounterReg(XPAR_TMRCTR_0_BASEADDR, 1) - timer_val_before; //Stores the time to execute the operation

     }

    //Prints the collected data
    for (i=0; i < NUMBER_OF_TRIALS; i++ ) {
    	xil_printf("%d,",i);
 		xil_printf("%d\n\r",numClockCycles[i]);
 		sum_of_cycles+= numClockCycles[i];
    }
    time_avg = sum_of_cycles/NUMBER_OF_TRIALS;
    event_avg = time_avg/number_of_events;
    //xil_printf("\n%d,%d,%d\n",sum_of_cycles,time_avg,event_avg);


    histogram(); //Creates a histogram for the measured data

}


void histogram(void){

	int min, max, binSize, binIndex;

	int i;

	//min and max initialized
	min = numClockCycles[0];
	max = numClockCycles[0];

	//find the min and max values
	for (i=0; i<NUMBER_OF_TRIALS; i++)
	{
		if (numClockCycles[i] < min) min = numClockCycles[i];
		if (numClockCycles[i] > max) max = numClockCycles[i];
	}

	binSize = (max - min)/NUMBER_OF_BINS;
	printf("Minimum: %d\n",min);
	printf("Bin Space: %d\n", binSize);
	printf("Maximum %d\n",max);
	//Bin number for each data element is found here
	for (i=0; i<NUMBER_OF_TRIALS; i++)
	{
		binIndex = 0;
		if (binSize > 0) {
			binIndex = (numClockCycles[i] - min)/binSize;
			if (binIndex >= NUMBER_OF_BINS) binIndex = NUMBER_OF_BINS - 1 ;
			histData[binIndex]++;
		}
		else {
			//if there is no variance in the data all values are assigned to bin 0
			histData[0] = NUMBER_OF_TRIALS;
			break;
		}
	}
	//Prints the number of elements in each bin
	for (i=0; i<NUMBER_OF_BINS; i++)
	{
		xil_printf("Bin %d: %d\n\r",i,histData[i]);
	}

	xil_printf("Done!\n\r");


}

