/*****************************************************************************
* bsp.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 27,2019
*****************************************************************************/

/**/
#include <stdio.h>
#include <math.h>
#include "qpn_port.h"
#include "bsp.h"
#include "lab2a.h"
#include "xintc.h"
#include "xil_exception.h"
#include "xparameters.h"	// Contains hardware addresses and bit masks
#include "xil_cache.h"		// Cache Drivers
#include "xtmrctr.h"		// Timer Drivers
#include "xtmrctr_l.h" 		// Low-level timer drivers
#include "xil_printf.h" 	// Used for xil_printf()
#include "xgpio.h" 		// LED driver, used for General purpose I/i
#include "xspi.h"
#include "xspi_l.h"
#include "lcd.h"

/*****************************/

/* Define all variables and Gpio objects here  */
XIntc sys_intctr;	//interrupt controller
XGpio enc_gpio; //encoder gpio
XGpio twist_Gpio; //encoder gpio
XGpio led_gpio; 	//16 LEDs
XGpio rgb_led_gpio;	//RGB leds
XTmrCtr sys_tmrctr;
XGpio dc;
XSpi spi;
XGpio btn;

#define RESET_VALUE 0x5F5E100
#define GPIO_CHANNEL1 1
#define LED_CHANNEL 1
#define BUTTON_CHANNEL 1

int toggle = 1;
int NumOfTri = 0;
int volume = 0;
int count = 0;
int TimerFlag = 0;
int EncTrig = 0;
static int encoder_count = 4;
static Xuint16 state = 0b11;
volatile u16 led_16 = 1;
int VolumeTimeOut = 0;
int TextTimeOut = 0;

static enum STATES {
		S0 = 0b11,
		S1 = 0b01,
		S2 = 0b00,
		S3 = 0b10
};

void debounceInterrupt(); // Write This function

// Create ONE interrupt controllers XIntc
// Create two static XGpio variables
// Suggest Creating two int's to use for determining the direction of twist

/* ----- New Timer Handler ----- */
void TimerCounterHandler(void *CallBackRef)
{
	if (MainVolumeFlag == 1) {
	if (VolumeFlag == 1) {
		setColor(0, 255, 0);
		fillRect(70, 90, act_volume+70, 110);
		VolumeTimeOut = 0;
		VolumeFlag = 0;
	}
	if (VolumeTimeOut > 3069) {
		printf("Timed Out");
		setColor(0, 0, 0);
		fillRect(70, 90, 170, 110);
		MainVolumeFlag = 0;
	}
	VolumeTimeOut++;
	}

	if (MainTextFlag == 1) {
		if (TextFlag == 1) {
			TextTimeOut = 0;
			TextFlag = 0;
		}
		if (TextTimeOut > 3069) {
			printf("Timed Out");
			RETEXT();
			MainTextFlag = 0;
		}
		TextTimeOut++;
	}
}

/*..........................................................................*/
void BSP_init(void) {
	XStatus Status;
	Status = XST_SUCCESS;
	u32 status;
	u32 controlReg;
	XSpi_Config *spiConfig;

	/* ----- Initialize Interrupt ----- */
	Status = XIntc_Initialize(&sys_intctr, XPAR_INTC_0_DEVICE_ID);

	/* ----- Initialize LED ----- */
	Status = XGpio_Initialize(&led_gpio, XPAR_AXI_GPIO_LED_DEVICE_ID);

	/* ----- Initialize RGB LED ----- */
	Status = XGpio_Initialize(&rgb_led_gpio, XPAR_AXI_GPIO_RGB_LED_DEVICE_ID);

	/* ----- Initialize ENCODER ----- */
	Status = XGpio_Initialize(&enc_gpio, XPAR_ENCODER_DEVICE_ID);

	XIntc_Connect(&sys_intctr, XPAR_MICROBLAZE_0_AXI_INTC_ENCODER_IP2INTC_IRPT_INTR,
			(Xil_ExceptionHandler)TwistHandler, &enc_gpio);
	XIntc_Enable(&sys_intctr, XPAR_MICROBLAZE_0_AXI_INTC_ENCODER_IP2INTC_IRPT_INTR);
	Status = XIntc_Start(&sys_intctr, XIN_REAL_MODE);
	XGpio_InterruptEnable(&enc_gpio, GPIO_CHANNEL1);
	XGpio_InterruptGlobalEnable(&enc_gpio);

	Status = XGpio_Initialize(&btn, XPAR_AXI_GPIO_BTN_DEVICE_ID);

	XIntc_Connect(&sys_intctr, XPAR_INTC_0_GPIO_1_VEC_ID,
			(Xil_ExceptionHandler)GpioHandler, &btn);
	XIntc_Enable(&sys_intctr, XPAR_INTC_0_GPIO_1_VEC_ID);
	Status = XIntc_Start(&sys_intctr, XIN_REAL_MODE);
	XGpio_InterruptEnable(&btn, BUTTON_CHANNEL);
	XGpio_InterruptGlobalEnable(&btn);

	/* ----- New Timer Setup ----- */
	Status = XTmrCtr_Initialize(&sys_tmrctr, XPAR_AXI_TIMER_0_DEVICE_ID);
	Status = XIntc_Initialize(&sys_intctr, XPAR_INTC_0_DEVICE_ID);
	Status = XIntc_Connect(&sys_intctr, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR,
				(XInterruptHandler)XTmrCtr_InterruptHandler,
				(void *)&sys_tmrctr);
	Status = XIntc_Start(&sys_intctr, XIN_REAL_MODE);
	XIntc_Enable(&sys_intctr, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR);
				microblaze_enable_interrupts();
	XTmrCtr_SetHandler(&sys_tmrctr, TimerCounterHandler, &sys_tmrctr);
	XTmrCtr_SetOptions(&sys_tmrctr, 0,
				XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
	XTmrCtr_SetResetValue(&sys_tmrctr, 0, 0xFFFF0000);
	XTmrCtr_Start(&sys_tmrctr, 0);



	status = XGpio_Initialize(&dc, XPAR_SPI_DC_DEVICE_ID);
	XGpio_SetDataDirection(&dc, 1, 0x0);
	spiConfig = XSpi_LookupConfig(XPAR_SPI_DEVICE_ID);
	status = XSpi_CfgInitialize(&spi, spiConfig, spiConfig->BaseAddress);
	XSpi_Reset(&spi);
	controlReg = XSpi_GetControlReg(&spi);
	XSpi_SetControlReg(&spi,
			(controlReg | XSP_CR_ENABLE_MASK | XSP_CR_MASTER_MODE_MASK) &
			(~XSP_CR_TRANS_INHIBIT_MASK));
	XSpi_SetSlaveSelectReg(&spi, ~0x01);

	/* ----- Initialize MICROBLAZE ----- */
	microblaze_register_handler((XInterruptHandler)XIntc_DeviceInterruptHandler,
			(void*)XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);
	microblaze_enable_interrupts();
}
/*..........................................................................*/
void QF_onStartup(void) {                 /* entered with interrupts locked */

/* Enable interrupts */
	xil_printf("\n\rQF_onStartup\n"); // Comment out once you are in your complete program

	initLCD();
	clrScr();
	//GREEN();
	AQUABLUE();

	// Variables for reading Microblaze registers to debug your interrupts.
//	{
//		u32 axi_ISR =  Xil_In32(intcPress.BaseAddress + XIN_ISR_OFFSET);
//		u32 axi_IPR =  Xil_In32(intcPress.BaseAddress + XIN_IPR_OFFSET);
//		u32 axi_IER =  Xil_In32(intcPress.BaseAddress + XIN_IER_OFFSET);
//		u32 axi_IAR =  Xil_In32(intcPress.BaseAddress + XIN_IAR_OFFSET);
//		u32 axi_SIE =  Xil_In32(intcPress.BaseAddress + XIN_SIE_OFFSET);
//		u32 axi_CIE =  Xil_In32(intcPress.BaseAddress + XIN_CIE_OFFSET);
//		u32 axi_IVR =  Xil_In32(intcPress.BaseAddress + XIN_IVR_OFFSET);
//		u32 axi_MER =  Xil_In32(intcPress.BaseAddress + XIN_MER_OFFSET);
//		u32 axi_IMR =  Xil_In32(intcPress.BaseAddress + XIN_IMR_OFFSET);
//		u32 axi_ILR =  Xil_In32(intcPress.BaseAddress + XIN_ILR_OFFSET) ;
//		u32 axi_IVAR = Xil_In32(intcPress.BaseAddress + XIN_IVAR_OFFSET);
//		u32 gpioTestIER  = Xil_In32(sw_Gpio.BaseAddress + XGPIO_IER_OFFSET);
//		u32 gpioTestISR  = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_ISR_OFFSET ) & 0x00000003; // & 0xMASK
//		u32 gpioTestGIER = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_GIE_OFFSET ) & 0x80000000; // & 0xMASK
//	}
}

void DrawBorder() {
	for (int width = 0; width<6;width++) {
		drawHLine(0, width, DISP_X_SIZE);
		drawHLine(0, DISP_Y_SIZE-width-10, DISP_X_SIZE);
		drawVLine(width, 0, DISP_Y_SIZE);
		drawVLine(DISP_X_SIZE-width, 0, DISP_Y_SIZE);
	}
}

void AQUABLUE() {
	clrScr();
	for (int Row = 0; Row<8; Row++) {
		int NewRow = Row*40;
		for (int Col = 0; Col<6; Col++) {
			int NewCol = Col*40;
			if ((Row == 1 || Row == 2 || Row == 3) && (Col == 1 || Col == 2 || Col == 3 || Col == 4)) {
				//Nothing
			} else {
			for (int y = 0; y<40; y++) {
				int blue = 2*ceil(y/2);
				setColor(0,255,255);
				drawHLine(0+NewCol, y+NewRow, 20-(blue/2));
				setColor(0, 0, 255);
				drawHLine(20-(blue/2)+NewCol, y+NewRow, blue);
				setColor(0,255,255);
				drawHLine(20+(blue/2)+NewCol, y+NewRow, 20-(blue/2));
			}
			}
		}
	}
	setColor(0, 255, 0);
	setColorBg(0, 0, 0);
	setFont(BigFont);
	lcdPrint("VOLUME", 73, 50);
}

void RETEXT() {
	setColor(0, 0, 0);
	setColorBg(0, 0, 0);
	setFont(SmallFont);
	lcdPrint("AAAAAAA", 100, 140);
}

void GREEN() {
	clrScr();
	for (int Row = 0; Row<8; Row++) {
		int NewRow = Row*40;
		if (Row%2 == 0) {
			NumOfTri = 6;
		} else {
			NumOfTri = 5;
		}
		for (int Col = 0; Col<NumOfTri; Col++) {
			int NewCol = Col*40;
			for (int y = 0; y<40; y++) {
				if (NumOfTri == 6) {
					int blue = 2*ceil(y/2);
					setColor(173,255,47);
					drawHLine(0+NewCol, y+NewRow, 20-(blue/2));
					setColor(50,205,50);
					drawHLine(20-(blue/2)+NewCol, y+NewRow, blue);
					setColor(173,255,47);
					drawHLine(20+(blue/2)+NewCol, y+NewRow, 20-(blue/2));
				} else {
					int blue = 2*ceil(y/2);
					setColor(173,255,47);
					drawHLine(20+NewCol, y+NewRow, 20-(blue/2));
					setColor(50,205,50);
					drawHLine(40-(blue/2)+NewCol, y+NewRow, blue);
					setColor(173,255,47);
					drawHLine(40+(blue/2)+NewCol, y+NewRow, 20-(blue/2));
					if (Col == 0 ) {
						drawHLine(0, y+NewRow, 20);
					}
					if (Col == NumOfTri-1) {
						drawHLine(DISP_X_SIZE-20, y+NewRow, 20);
					}
				}
			}
		}
	}
	setColor(50,205,50);
	DrawBorder();
}

void TEXTTIMER() {
	TimerFlag = 0;
	for(count = 0; count < 10000000; count++) {
		if (TimerFlag == 1) {
			return;
		}
	}
	RETEXT();
}



void QF_onIdle(void) {        /* entered with interrupts locked */

    QF_INT_UNLOCK();                       /* unlock interrupts */

    {
    	// Write code to increment your interrupt counter here.
    	// QActive_postISR((QActive *)&AO_Lab2A, ENCODER_DOWN); is used to post an event to your FSM



// 			Useful for Debugging, and understanding your Microblaze registers.
//    		u32 axi_ISR =  Xil_In32(intcPress.BaseAddress + XIN_ISR_OFFSET);
//    	    u32 axi_IPR =  Xil_In32(intcPress.BaseAddress + XIN_IPR_OFFSET);
//    	    u32 axi_IER =  Xil_In32(intcPress.BaseAddress + XIN_IER_OFFSET);
//
//    	    u32 axi_IAR =  Xil_In32(intcPress.BaseAddress + XIN_IAR_OFFSET);
//    	    u32 axi_SIE =  Xil_In32(intcPress.BaseAddress + XIN_SIE_OFFSET);
//    	    u32 axi_CIE =  Xil_In32(intcPress.BaseAddress + XIN_CIE_OFFSET);
//    	    u32 axi_IVR =  Xil_In32(intcPress.BaseAddress + XIN_IVR_OFFSET);
//    	    u32 axi_MER =  Xil_In32(intcPress.BaseAddress + XIN_MER_OFFSET);
//    	    u32 axi_IMR =  Xil_In32(intcPress.BaseAddress + XIN_IMR_OFFSET);
//    	    u32 axi_ILR =  Xil_In32(intcPress.BaseAddress + XIN_ILR_OFFSET) ;
//    	    u32 axi_IVAR = Xil_In32(intcPress.BaseAddress + XIN_IVAR_OFFSET);
//
//    	    // Expect to see 0x00000001
//    	    u32 gpioTestIER  = Xil_In32(sw_Gpio.BaseAddress + XGPIO_IER_OFFSET);
//    	    // Expect to see 0x00000001
//    	    u32 gpioTestISR  = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_ISR_OFFSET ) & 0x00000003;
//
//    	    // Expect to see 0x80000000 in GIER
//    		u32 gpioTestGIER = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_GIE_OFFSET ) & 0x80000000;


    }
}

/* Do not touch Q_onAssert */
/*..........................................................................*/
void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line) {
    (void)file;                                   /* avoid compiler warning */
    (void)line;                                   /* avoid compiler warning */
    QF_INT_LOCK();
    for (;;) {
    }
}

/* Interrupt handler functions here.  Do not forget to include them in lab2a.h!
To post an event from an ISR, use this template:
QActive_postISR((QActive *)&AO_Lab2A, SIGNALHERE);
Where the Signals are defined in lab2a.h  */

/******************************************************************************
*
* This is the interrupt handler routine for the GPIO for this example.
*
******************************************************************************/
void TwistHandler(void *CallbackRef) {
	XGpio *GpioPtr = (XGpio *)CallbackRef;
	Xuint32 encoderStatus = 0;

	Xuint32 start = XTmrCtr_GetTimerCounterReg(sys_tmrctr.BaseAddress, 0);
	Xuint32 finish = start;

	while (finish < (start + RESET_VALUE/10000)) {
		finish = XTmrCtr_GetTimerCounterReg(sys_tmrctr.BaseAddress, 0);
	}

	encoderStatus = XGpio_DiscreteRead(&enc_gpio, 1);

	if (encoderStatus == 7) {
		state = S0;
		encoder_count = 4;
		QActive_postISR((QActive *)&AO_Lab2A, ENCODER_CLICK);
	}

		switch (state) {

			case S0: {
					if (encoder_count == 8 || encoder_count == 0) {
						encoder_count = 4;
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
							QActive_postISR((QActive *)&AO_Lab2A, ENCODER_DOWN);
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
							QActive_postISR((QActive *)&AO_Lab2A, ENCODER_UP);
						}
						break;
					}
					break;
				}
				break;
			}
			break;
		}
		XGpio_InterruptClear(GpioPtr, GPIO_CHANNEL1);	// Clearing interrupt
}

void GpioHandler(void *CallbackRef) {
	XGpio *GpioPtr = (XGpio *)CallbackRef;

	XGpio_InterruptClear(GpioPtr, BUTTON_CHANNEL);	// Clearing interrupt

	Xuint32 start = XTmrCtr_GetTimerCounterReg(sys_tmrctr.BaseAddress, 0);
	Xuint32 finish = start;

	while (finish < (start + RESET_VALUE/10000)) {
		finish = XTmrCtr_GetTimerCounterReg(sys_tmrctr.BaseAddress, 0);
	}

	Xuint32 ButtonPressStatus = 0;
	ButtonPressStatus = XGpio_DiscreteRead(&btn, BUTTON_CHANNEL); // Check GPIO output
	if (ButtonPressStatus == 0x04) {
		//RIGHT
		print("Button Pressed");
		//TimerFlag = 1;
		QActive_postISR((QActive *)&AO_Lab2A, BTNR);
	}
	else if (ButtonPressStatus == 0x02) {
		//LEFT
		print("Button Pressed");
		//TimerFlag = 1;
		QActive_postISR((QActive *)&AO_Lab2A, BTNL);
	}
	else if (ButtonPressStatus == 0x10) {
		//CENTER
		print("Button Pressed");
		//TimerFlag = 1;
		QActive_postISR((QActive *)&AO_Lab2A, BTNC);
	}
	else if (ButtonPressStatus == 0x01) {
		//Up
		print("Button Pressed");
		//TimerFlag = 1;
		QActive_postISR((QActive *)&AO_Lab2A, BTNU);
	}
	else if (ButtonPressStatus == 0x08) {
		//Down
		print("Button Pressed");
		//TimerFlag = 1;
		QActive_postISR((QActive *)&AO_Lab2A, BTND);
	}
}
