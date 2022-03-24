#include "note.h"
//#include "lcd.h"

//array to store note names for findNote
static char notes[12][3]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

//finds and prints note of frequency and deviation from note
void findNote(float f) {
	float c=261.63;
	float r;
	int oct=4;
	int note=0;
	//determine which octave frequency is in
	if(f >= c) {
		while(f > c*2) {
			c=c*2;
			oct++;
		}
	}
	else { //f < C4
		while(f < c) {
			c=c/2;
			oct--;
		}
	
	}

	//find note below frequency
	//c=middle C
	r=c*root2;
	while(f > r) {
		c=c*root2;
		r=r*root2;
		note++;
	}

   /*
	//determine which note frequency is closest to
	if((f-c) <= (r-f)) { //closer to left note
		WriteString("N:");
		WriteString(notes[note]);
		WriteInt(oct);
		WriteString(" D:+");
		WriteInt((int)(f-c+.5));
		WriteString("Hz");
	}
	else { //f closer to right note
		note++;
		if(note >=12) note=0;
		WriteString("N:");
		WriteString(notes[note]);
		WriteInt(oct);
		WriteString(" D:-");
		WriteInt((int)(r-f+.5));
		WriteString("Hz");
	}
   */
}
