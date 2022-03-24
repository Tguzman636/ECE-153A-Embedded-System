#include <stdio.h>
#include "xil_printf.h"
#include "xil_cache.h"
#include "sevenSeg_new.h"
#include "xintc.h"
#include "xtmrctr.h"
#include "xtmrctr_l.h"
#include "xparameters.h"
#include <xbasic_types.h>
#include "xgpio.h"

#define RESET_VALUE 0xF4240 // 1*10^6 @ 100MHz = 10ms
#define BUTTON_CHANNEL 1

XTmrCtr sys_tmrctr;
XIntc sys_intctr;
XGpio sys_gpio;
unsigned int count = 0;

static u16 GlobalIntrMask;	// GPIO Interrupt Handler Mask
static volatile u32 Start = 0;	// Interrupt Handler Flag
static volatile u32 Direct = 1;
volatile int num[8];

void reset_num() //resets the numbers back to zero
{
	for (int i = 0; i < 8; i++) {
		    	num[i] = 0;
		    }
}

void update_count()
{
	if (num[7] == 0 && num[6] == 0 && num[5] == 0 && num[4] == 0 && num[3] == 0 && num[2] == -1)
	{
		Start = 0;
		Direct = 1;
		reset_num();
	}
	for (int i = 0; i < 7; i++)
	{
		if (num[i] > 9)
		{
			num[i+1] += 1;
			num[i] = 0;
		}
		if (num[i] < 0)
		{
			num[i+1] -= 1;
			num[i] = 9;
		}
	}

	if (num[7] > 9)
	{
		Start = 0;
	    for (int i = 0; i < 8; i++) {
	    	num[i] = 0;
	    }
	}

}

void update_num()
{
	sevenseg_draw_digit(0,num[0]);
	sevenseg_draw_digit(1,num[1]);
	sevenseg_draw_digit(2,num[2]);
	sevenseg_draw_digit(3,num[3]);
	sevenseg_draw_digit(4,num[4]);
	sevenseg_draw_digit(5,num[5]);
	sevenseg_draw_digit(6,num[6]);
	sevenseg_draw_digit(7,num[7]);
}

void timer_handler()	// updates the stop-watch array
{
	Xuint32 ControlStatusReg;

	ControlStatusReg = XTimerCtr_ReadReg(sys_tmrctr.BaseAddress, 0, XTC_TCSR_OFFSET);

	int min = 2;
	if (Start == 1) {
		if (Direct == 1) {
			num[min]++;
			if (num[min] > 9) {
				update_count();
			}
		} else if (Direct == 0) {
			num[min]--;
			if (num[min]<0) {
				update_count();
			}
		}
	}

	XTmrCtr_WriteReg(sys_tmrctr.BaseAddress, 0, XTC_TCSR_OFFSET, ControlStatusReg |XTC_CSR_INT_OCCURED_MASK); // clears interrupt

}

void gpio_handler(void *CallbackRef)
{
	XGpio *GpioPtr = (XGpio *)CallbackRef;

	XGpio_InterruptClear(GpioPtr, GlobalIntrMask);	// Clearing interrupt

	Xuint32 ButtonPressStatus = 0;
	ButtonPressStatus = XGpio_DiscreteRead(&sys_gpio, BUTTON_CHANNEL); // Check GPIO output
	if (ButtonPressStatus == 0x04)
	{
		//RIGHT: Stop
		Start = 0;
	}
	else if (ButtonPressStatus == 0x02)
	{
		//LEFT: Start
		Start = 1;
	}
	else if (ButtonPressStatus == 0x10)
	{
		//CENTER: Reset
		reset_num();
	}
	else if (ButtonPressStatus == 0x01)
	{
		//Up: Direction up
		Direct = 1;

		/*Bouncing and Glitch */

		/*if (Direct == 1) {
			Direct = 0;
		} else if (Direct == 0) {
			Direct = 1;
		}
		*/
	}
	else if (ButtonPressStatus == 0x08)
	{
		//Down: Direction down
		Direct = 0;
	}

}

void initialize()
{
	XStatus Status;
	Status = XST_SUCCESS;

	Status = XIntc_Initialize(&sys_intctr, XPAR_INTC_0_DEVICE_ID);
	if ( Status != XST_SUCCESS )
	{
		xil_printf("Timer interrupt initialization failed...\r\n");
		return XST_FAILURE;
	}

	Status = XIntc_Connect(&sys_intctr, XPAR_INTC_0_TMRCTR_0_VEC_ID,
			(XInterruptHandler)timer_handler, &sys_tmrctr);
	if ( Status != XST_SUCCESS )
	{
		xil_printf("Failed to connect the application handlers to the interrupt controller...\r\n");
		return XST_FAILURE;
	}

	Status = XIntc_Start(&sys_intctr, XIN_REAL_MODE);
	if ( Status != XST_SUCCESS )
	{
		xil_printf("Interrupt controller driver failed to start...\r\n");
		return XST_FAILURE;
	}

	XIntc_Enable(&sys_intctr, XPAR_INTC_0_TMRCTR_0_VEC_ID);

	Status = XTmrCtr_Initialize(&sys_tmrctr, XPAR_INTC_0_DEVICE_ID);
	if ( Status != XST_SUCCESS )
	{
		xil_printf("Timer initialization failed...\r\n");
		return XST_FAILURE;
	}

	XTmrCtr_SetOptions(&sys_tmrctr, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
	XTmrCtr_SetResetValue(&sys_tmrctr, 0, 0xFFFFFFFF-RESET_VALUE);
	XTmrCtr_Start(&sys_tmrctr, 0);

	microblaze_register_handler((XInterruptHandler)XIntc_DeviceInterruptHandler,
				(void*)XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);
	microblaze_enable_interrupts();

	if ( Status != XST_SUCCESS ){
	    xil_printf("Initialization failed.\r\n");
	    return XST_FAILURE;
	}
	xil_printf("Initialize TIMER Success.\r\n");

	Status = XST_SUCCESS;

	Status = XGpio_Initialize(&sys_gpio, XPAR_AXI_GPIO_BTN_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("sys_gpio initialization failed...\r\n");
		return XST_FAILURE;
	}

	XIntc_Connect(&sys_intctr, XPAR_INTC_0_GPIO_0_VEC_ID,
			(Xil_ExceptionHandler)gpio_handler, &sys_gpio);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	XIntc_Enable(&sys_intctr, XPAR_INTC_0_GPIO_0_VEC_ID);
	Status = XIntc_Start(&sys_intctr, XIN_REAL_MODE);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	XGpio_InterruptEnable(&sys_gpio, 0x1);
	XGpio_InterruptGlobalEnable(&sys_gpio);

	if ( Status != XST_SUCCESS ){
	    xil_printf("Initialization failed.\r\n");
	    return XST_FAILURE;
	}
	xil_printf("Initialize GPIO Success.\r\n");
}


int main()
{
	Xil_ICacheInvalidate();
	Xil_ICacheEnable();
	Xil_DCacheInvalidate();
	Xil_DCacheEnable();

    // initialize timer/gpio interrupt
    initialize();

    // initialize array
    reset_num();

    // Keeps number updated
    while(1)
    {
    	update_num();
    }
}
