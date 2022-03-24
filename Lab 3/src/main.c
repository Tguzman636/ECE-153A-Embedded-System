/*
 * Copyright (c) 2009-2012 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "xil_cache.h"
#include <mb_interface.h>

#include "xparameters.h"
#include <xil_types.h>
#include <xil_assert.h>

#include <xio.h>
#include "xtmrctr.h"
#include "xintc.h"
#include "fft.h"
#include "note.h"
#include "stream_grabber.h"
#include <xbasic_types.h>

#define SAMPLES 512 // AXI4 Streaming Data FIFO has size 512
#define M 9 //2^m=samples
#define CLOCK 100000000.0 //clock speed

int tenth_avg = 0;
int reducer = 2;
int reducer_sq = 4;
int temp_avg = 0;
int avg_count = 0;
static float q[SAMPLES];
static float w[SAMPLES];

// Performance analysis code

XTmrCtr tmr;
XIntc sys_intctr;
struct Performance Performance;

struct Performance
{
	unsigned fft;
	unsigned read_fsl_values;
	unsigned other;
	unsigned main;
	unsigned findNote;
};


void performanceAnalyzer()
{
	/*
	 * polls the register_14 every 1ms to see where
	 * most of the time is spent in the while loop
	 *
	 * */
	Xuint32 a;
	asm("add %0, r0,r14" : "= r"(a));
	if(a >= 0x80012ac4 && a < 0x800135d8)
	{
		Performance.fft++;
	}
	else if ( a >= 0x800138c4 && a < 0x80013a8c)
	{
		Performance.read_fsl_values++;
	}
	else if ( a >= 0x80013a78 && a < 0x80013edc)
	{
		Performance.main++;
	}
	else if ( a >= 0x80013ee0 && a < 0x8001411c)
	{
		Performance.findNote++;
	}
	else
	{
		Performance.other++;
	}
}

void read_fsl_values(float* q, int n) {
   int i;
   unsigned int x;
   int avg_add;
   stream_grabber_start();
   stream_grabber_wait_enough_samples(512);

   avg_add = 0;

   for(i = 0; i < n; i++) {
      avg_add = avg_add + stream_grabber_read_sample(i);
      if ((i+1) % reducer_sq == 0)
      {
    	  x = avg_add/reducer_sq;
    	  q[i/reducer_sq] = 3.3*x/67108864.0; // 3.3V and 2^26 bit precision.
    	  avg_add = 0;
      }
   }
}

void timer_handler()
{
	Xuint32 ControlStatusReg;
	ControlStatusReg = XTimerCtr_ReadReg(tmr.BaseAddress, 1, XTC_TCSR_OFFSET);
	performanceAnalyzer();
	XTmrCtr_WriteReg(tmr.BaseAddress, 1, XTC_TCSR_OFFSET, ControlStatusReg |XTC_CSR_INT_OCCURED_MASK); // clears interrupt
}

void Performance_Timer()
{
	XIntc_Initialize(&sys_intctr, XPAR_INTC_0_DEVICE_ID);
	XIntc_Connect(&sys_intctr, XPAR_INTC_0_TMRCTR_0_VEC_ID,
			(XInterruptHandler)timer_handler, &tmr);
	XIntc_Start(&sys_intctr, XIN_REAL_MODE);
	XIntc_Enable(&sys_intctr, XPAR_INTC_0_TMRCTR_0_VEC_ID);
	XTmrCtr_Initialize(&tmr, XPAR_INTC_0_DEVICE_ID);
	XTmrCtr_SetOptions(&tmr, 1, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
	XTmrCtr_SetResetValue(&tmr, 1, 0xFFFFFFFF-0x186A0);
	XTmrCtr_Start(&tmr, 1);
	microblaze_register_handler((XInterruptHandler)XIntc_DeviceInterruptHandler,
				(void*)XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);
	microblaze_enable_interrupts();

}

int main() {
   float sample_f;
   int l;
   int ticks; //used for timer
   uint32_t Control;
   float frequency;
   float tot_time; //time to run program

   Xil_ICacheInvalidate();
   Xil_ICacheEnable();
   Xil_DCacheInvalidate();
   Xil_DCacheEnable();

   //set up timer
   XTmrCtr timer;
   XTmrCtr_Initialize(&timer, XPAR_AXI_TIMER_0_DEVICE_ID);
   Control = XTmrCtr_GetOptions(&timer, 0) | XTC_CAPTURE_MODE_OPTION | XTC_INT_MODE_OPTION;
   XTmrCtr_SetOptions(&timer, 0, Control);

   Initialize();

   // performance timer
   Performance_Timer();

   print("Program started\n\r");

   while(1) { 
      XTmrCtr_Start(&timer, 0);
      XTmrCtr_Start(&tmr, 1);

      //Read Values from Microblaze buffer, which is continuously populated by AXI4 Streaming Data FIFO.
      read_fsl_values(q, SAMPLES);

      sample_f = 100*1000*1000/2048.0;

      //zero w array
      for(l=0;l<SAMPLES;l++)
         w[l]=0; 

      frequency=fft(q,w,SAMPLES/reducer_sq,M-reducer,sample_f/reducer_sq);
      ticks=XTmrCtr_GetValue(&timer, 0);
      XTmrCtr_Stop(&timer, 0);
      XTmrCtr_Stop(&tmr, 1);
      tot_time=ticks/CLOCK;

      //ignore noise below set frequency
      if(frequency > 180.0) {
         findNote(frequency);
         //xil_printf("fft %d\n\r", Performance.fft);
         //xil_printf("read_fsl_values %d\n\r", Performance.read_fsl_values);
         // xil_printf("main %d\n\r", Performance.main);
         // xil_printf("findNote %d\n\r", Performance.findNote);
         //xil_printf("other %d\n\r", Performance.other);
         // program print
         xil_printf("frequency: %d Hz\r\n", (int)(frequency));
         if (avg_count == 10) {
        	 tenth_avg = temp_avg/10;
        	 avg_count = 0;
        	 temp_avg = 0;
         }
         xil_printf("avg freq (10th samp): %dHz \r\n",(int)(tenth_avg));
         xil_printf("program time: %dms \r\n",(int)(1000*tot_time));
         temp_avg += frequency;
         avg_count++;
      }
      Performance.fft = 0;
      Performance.read_fsl_values =0;
      Performance.main = 0;
      Performance.findNote = 0;
      Performance.other = 0;

   }


   return 0;
}
