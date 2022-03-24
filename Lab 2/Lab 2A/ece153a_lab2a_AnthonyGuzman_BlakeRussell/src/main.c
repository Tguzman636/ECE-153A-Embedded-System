#include <stdio.h>
#include "xil_cache.h"
#include "xintc.h"
#include "xtmrctr.h"
#include "xtmrctr_l.h"
#include "xparameters.h"
#include <xbasic_types.h>
#include "xgpio.h"
#include "xil_printf.h"

#define RESET_VALUE 0x5F5E100 // 100*10^6 @ 100MHz = 1s
#define ENCODER_CHANNEL 1
#define LED_CHANNEL 1


XTmrCtr sys_tmrctr; //timer
XIntc sys_intctr;	//interrupt controller
XGpio enc_gpio; //encoder gpio
XGpio led_gpio; 	//16 LEDs
XGpio rgb_led_gpio;	//RGB leds

unsigned int count = 0;	//count var for the timing loop
static int encoder_count = 4;
unsigned int toggle = 1;
static Xuint16 state = 0b11;
volatile u16 led_16 = 1;

static enum STATES {
		S0 = 0b11,
		S1 = 0b01,
		S2 = 0b00,
		S3 = 0b10
};

void LED_press(int toggle) {
	if (toggle == 0) {
		XGpio_DiscreteWrite(&led_gpio, LED_CHANNEL, 0x00);
	} else {
		XGpio_DiscreteWrite(&led_gpio, LED_CHANNEL, led_16);
	}
}

void left_overflow() {
	if(led_16 == 0b1000000000000000) {
		led_16 = 0b1;
	} else {
		led_16 = led_16 << 1;
	}
	XGpio_DiscreteWrite(&led_gpio, LED_CHANNEL, led_16);
}

void right_overflow() {
	if(led_16 == 0b1) {
		led_16 = 0b1000000000000000;
	} else {
		led_16 = led_16 >> 1;
	}
	XGpio_DiscreteWrite(&led_gpio, LED_CHANNEL, led_16);
}

void timer_handler() {
	Xuint32 ControlStatusReg;
	ControlStatusReg = XTimerCtr_ReadReg(sys_tmrctr.BaseAddress, 0, XTC_TCSR_OFFSET);
	XTmrCtr_WriteReg(sys_tmrctr.BaseAddress, 0, XTC_TCSR_OFFSET, ControlStatusReg |XTC_CSR_INT_OCCURED_MASK);

}

void enc_handler(void *CallbackRef) {
	XGpio *GpioPtr = (XGpio *)CallbackRef;

	Xuint32 encoderStatus = 0;

	Xuint32 start = XTmrCtr_GetTimerCounterReg(sys_tmrctr.BaseAddress, 0);
	Xuint32 finish = start;

	while (finish < (start + RESET_VALUE/10000)) {
		finish = XTmrCtr_GetTimerCounterReg(sys_tmrctr.BaseAddress, 0);
	}

	encoderStatus = XGpio_DiscreteRead(&enc_gpio, ENCODER_CHANNEL);

	if (encoderStatus == 7) {
		if (toggle == 0) {
			toggle = 1;
		} else {
			toggle = 0;
		}
		state = S0;
		encoder_count = 4;
		LED_press(toggle);
	}

	switch (state) {

		case S0: {
			if (toggle == 1) {
				if (encoder_count == 8 || encoder_count == 0) {
					encoder_count = 4;
				}
			}
			switch(encoderStatus) {
				case 0b01: {
					if(encoder_count == 4) {
						encoder_count = encoder_count + 1;
						state = S1;
					}
					break;
				}
				case 0b10: {
					if(encoder_count == 4) {
						encoder_count = encoder_count - 1;
						state = S3;
					}
					break;
				}
				break;
			}
			break;
		}

		case S1: {
			switch(encoderStatus) {
				case 0b11: {
					if(encoder_count == 1) {
						encoder_count = encoder_count - 1;
						state = S0;
						if (toggle == 1) {
							left_overflow();
						}
					}
					break;
				}
				case 0b00: {
					if(encoder_count == 5) {
						encoder_count = encoder_count + 1;
						state = S2;
					}
					break;
				}
				break;
			}
			break;
		}

		case S2: {
			switch(encoderStatus) {
				case 0b01: {
					if (encoder_count == 2) {
						encoder_count = encoder_count - 1;
						state = S1;
					}
					break;
				}
				case 0b10: {
					if (encoder_count == 6) {
						encoder_count = encoder_count + 1;
						state = S3;
					}
					break;
				}
				break;
			}
			break;
		}

		case S3: {
			switch(encoderStatus) {
				case 0b00: {
					if(encoder_count == 3) {
						encoder_count = encoder_count - 1;
						state = S2;
					}
					break;
				}
				case 0b11: {
					if(encoder_count == 7) {
						encoder_count = encoder_count + 1;
						state = S0;
						if (toggle == 1) {
							right_overflow();
						}
					}
					break;
				}
				break;
			}
			break;
		}
		break;
	}
	//XGpio_DiscreteWrite(&led_gpio, LED_CHANNEL, encoder_count);   //for debugging purposes
	XGpio_InterruptClear(GpioPtr, ENCODER_CHANNEL);	// Clearing interrupt
}


int initialization()
{
	XStatus Status;
	Status = XST_SUCCESS;

	/* ----- Initialize Interrupt ----- */
	Status = XIntc_Initialize(&sys_intctr, XPAR_INTC_0_DEVICE_ID);
	Status = XIntc_Connect(&sys_intctr, XPAR_INTC_0_TMRCTR_0_VEC_ID,
			(XInterruptHandler)timer_handler, &sys_tmrctr);
	Status = XIntc_Start(&sys_intctr, XIN_REAL_MODE);
	XIntc_Enable(&sys_intctr, XPAR_INTC_0_TMRCTR_0_VEC_ID);

	/* ----- Initialize Timer ----- */
	Status = XTmrCtr_Initialize(&sys_tmrctr, XPAR_INTC_0_DEVICE_ID);
	XTmrCtr_SetOptions(&sys_tmrctr, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
	XTmrCtr_SetResetValue(&sys_tmrctr, 0, 0xFFFFFFFF-RESET_VALUE);
	XTmrCtr_Start(&sys_tmrctr, 0);

	/* ----- Initialize LED ----- */
	Status = XGpio_Initialize(&led_gpio, XPAR_AXI_GPIO_LED_DEVICE_ID);

	/* ----- Initialize RGB LED ----- */
	Status = XGpio_Initialize(&rgb_led_gpio, XPAR_AXI_GPIO_RGB_LED_DEVICE_ID);

	/* ----- Initialize ENCODER ----- */
	Status = XGpio_Initialize(&enc_gpio, XPAR_ENCODER_DEVICE_ID);
	XIntc_Connect(&sys_intctr, XPAR_MICROBLAZE_0_AXI_INTC_ENCODER_IP2INTC_IRPT_INTR,
			(Xil_ExceptionHandler)enc_handler, &enc_gpio);
	XIntc_Enable(&sys_intctr, XPAR_MICROBLAZE_0_AXI_INTC_ENCODER_IP2INTC_IRPT_INTR);
	Status = XIntc_Start(&sys_intctr, XIN_REAL_MODE);
	XGpio_InterruptEnable(&enc_gpio, ENCODER_CHANNEL);
	XGpio_InterruptGlobalEnable(&enc_gpio);

	/* ----- Initialize MICROBLAZE ----- */
	microblaze_register_handler((XInterruptHandler)XIntc_DeviceInterruptHandler,
				(void*)XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);

	microblaze_enable_interrupts();
	return Status;
}

void rgb_blink(){
	XGpio_DiscreteWrite(&rgb_led_gpio, LED_CHANNEL, 0x2);  //1 = blue; 2 = green; 4 = red
	for(count = 0; count < 5000000; count++);
	XGpio_DiscreteWrite(&rgb_led_gpio, LED_CHANNEL, 0x0);
	for(count = 0; count < 5000000; count++);
}

int main()
{
	Xil_ICacheInvalidate();
	Xil_ICacheEnable();
	Xil_DCacheInvalidate();
	Xil_DCacheEnable();

    print("----Entering Main----\n\r");

    initialization();

    XGpio_DiscreteWrite(&led_gpio, LED_CHANNEL, 1 << led_16);

    while(1)
    {
    	rgb_blink();
    }

    return XST_SUCCESS;
}

