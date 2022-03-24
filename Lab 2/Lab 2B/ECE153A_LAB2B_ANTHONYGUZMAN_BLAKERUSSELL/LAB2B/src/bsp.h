/*****************************************************************************
* bsp.h for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/
#ifndef bsp_h
#define bsp_h


                                              
extern int act_volume;
extern int MainVolumeFlag;
extern int VolumeFlag;
extern int MainTextFlag;
extern int TextFlag;

/* bsp functions ..........................................................*/

void BSP_init(void);
void ISR_gpio(void);
void ISR_timer(void);

#define BSP_showState(prio_, state_) ((void)0)


#endif                                                             /* bsp_h */


