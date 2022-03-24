/*****************************************************************************
* lab2a.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/

#define AO_LAB2A

#include "qpn_port.h"
#include "bsp.h"
#include "lab2a.h"
#include "lcd.h"
#include <math.h>




typedef struct Lab2ATag  {               //Lab2A State machine
	QActive super;
}  Lab2A;

int act_volume = 0;
int stored_value = 0;
int alive = 1;
int Mute = 0;
int MainVolumeFlag = 0;
int VolumeFlag = 0;
int MainTextFlag = 0;
int TextFlag = 0;
/* Setup state machines */
/**********************************************************************/
static QState Lab2A_initial (Lab2A *me);
static QState Lab2A_on      (Lab2A *me);
static QState Lab2A_stateA  (Lab2A *me);
static QState Lab2A_stateB  (Lab2A *me);


/**********************************************************************/


Lab2A AO_Lab2A;


void Lab2A_ctor(void)  {
	Lab2A *me = &AO_Lab2A;
	QActive_ctor(&me->super, (QStateHandler)&Lab2A_initial);
}


QState Lab2A_initial(Lab2A *me) {
	xil_printf("\n\rInitialization");
    return Q_TRAN(&Lab2A_on);
}

QState Lab2A_on(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			xil_printf("\n\rOn");
			}
			
		case Q_INIT_SIG: {
			return Q_TRAN(&Lab2A_stateA);
			}
	}
	
	return Q_SUPER(&QHsm_top);
}

void VolumeUp() {
	printf("Volume: %d\n", act_volume);
	setColor(0, 255, 0);
	fillRect(act_volume+69, 90, act_volume+70, 110);
	MainVolumeFlag = 1;
	VolumeFlag = 1;
}

void VolumeDown() {
	printf("Volume: %d\n", act_volume);
	setColor(0, 0, 0);
	fillRect(act_volume+70, 90, act_volume+71, 110);
	MainVolumeFlag = 1;
	VolumeFlag = 1;
}

void VolumeToggle(int toggle) {
	if (toggle == 1) {
		setColor(0, 0, 0);
		fillRect(70, 90, 170, 110);
	} else {
		setColor(0, 255, 0);
		fillRect(69, 90, 70+act_volume, 110);
		MainVolumeFlag = 1;
		VolumeFlag = 1;
	}
}

void OnScreenText(char* s1) {
	setColor(255, 0, 255);
	setColorBg(0, 0, 0);
	setFont(SmallFont);
	lcdPrint(s1, 100, 140);
	MainTextFlag = 1;
	TextFlag = 1;
	//TEXTTIMER();
}

void TimeOutVOLUME() {

}

/* Create Lab2A_on state and do any initialization code if needed */
/******************************************************************/

QState Lab2A_stateA(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			xil_printf("Startup State A\n");
			return Q_HANDLED();
		}

		case ENCODER_UP: {
			xil_printf("Encoder Up from State A\n");
			if (act_volume < 100) {
				act_volume ++;
			} else if (act_volume >= 100) {		//Worst case if bugged
				act_volume = 100;
			}
			VolumeUp();
			return Q_HANDLED();
		}

		case ENCODER_DOWN: {
			xil_printf("Encoder Down from State A\n");
			if (act_volume > 0) {
				act_volume --;
			} else if (act_volume <= 0) {		//Worst case if bugged
				act_volume = 0;
			}
			VolumeDown();
			return Q_HANDLED();
		}

		case ENCODER_CLICK:  {
			xil_printf("Changing State\n");
			Mute = 1;
			VolumeToggle(Mute);
			return Q_TRAN(&Lab2A_stateB);
		}
		case BTNU: { // UP
			xil_printf("BTNU from State A\n");
			OnScreenText("MODE:1");
			return Q_HANDLED();
		}
		case BTND: { // DOWN
			xil_printf("BTND from State A\n");
			OnScreenText("MODE:2");
			return Q_HANDLED();
		}
		case BTNC: { // CENTER
			xil_printf("BTNC from State A\n");
			OnScreenText("MODE:3");
			return Q_HANDLED();
		}
		case BTNL: { // LEFT
			xil_printf("BTNL from State A\n");
			OnScreenText("MODE:4");
			return Q_HANDLED();
		}
		case BTNR: { // RIGHT
			xil_printf("BTNR from State A\n");
			OnScreenText("MODE:5");
			return Q_HANDLED();
		}

	}

	return Q_SUPER(&Lab2A_on);

}

QState Lab2A_stateB(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			xil_printf("Startup State B\n");
			return Q_HANDLED();
		}

		case ENCODER_UP: {
			xil_printf("Encoder Up from State B\n");
			return Q_HANDLED();
		}

		case ENCODER_DOWN: {
			xil_printf("Encoder Down from State B\n");
			return Q_HANDLED();
		}

		case ENCODER_CLICK:  {
			xil_printf("Changing State\n");
			Mute = 0;
			VolumeToggle(Mute);
			return Q_TRAN(&Lab2A_stateA);
		}
		case BTNU: { // UP
			OnScreenText("MODE:1");
			return Q_HANDLED();
		}
		case BTND: { // DOWN
			OnScreenText("MODE:2");
			return Q_HANDLED();
		}
		case BTNC: { // CENTER
			OnScreenText("MODE:3");
			return Q_HANDLED();
		}
		case BTNL: { // LEFT
			OnScreenText("MODE:4");
			return Q_HANDLED();
		}
		case BTNR: { // RIGHT
			OnScreenText("MODE:5");
			return Q_HANDLED();
		}

	}

	return Q_SUPER(&Lab2A_on);

}

