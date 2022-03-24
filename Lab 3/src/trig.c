#include "trig.h"
#include <stdio.h>

#define K 256
#define B 10
static float cos_table[K][B];
static float sin_table[K][B];

int factorial(int a) {
	if(a==0) return 1;
	return a*factorial(a-1);
}

float cosine(float);

float sine(float x) {
	if(x > (PI/2) || x < (-PI/2)){
		float d=x/2;
		return cosine(d)*sine(d)*2;
	}
	int i,j;
	float sine=0;
	float power;
	for(i=0;i<10;i++) {
		power=x;
		if(i!=0) {
			for(j=0;j<i*2;j++)
				power*=x;
		}
		if(i%2==1)
			power*=-1;
		sine+=power/factorial(2*i+1);
	}
	return sine;
}

float cosine(float x){
	float c,s;
	if(x > (PI/2) || x < (-PI/2)) {
		c=cosine(x/2);
		s=sine(x/2);
		return c*c-s*s;
	}
	int i,j;
	float cosine=0;
	float power;
	for(i=0;i<10;i++) {
		if(i==0) power=1;
		else power=x;
		if(i!=0) {
			for(j=0;j<i*2-1;j++)
				power*=x;
		}
		if(i%2==1)
			power*=-1;
		cosine+=power/factorial(2*i);
	}
	return cosine;
}

void Initialize() {
	int b = 1;
	for (int r = 0; r < K; r++)
	{
		cos_table[r][0] = 0;
		sin_table[r][0] = 0;
		for (int c = 1; c < B; c++)
		{
			cos_table[r][c] = cosine(-PI*r/b);
			sin_table[r][c] = sine(-PI*r/b);
			b *= 2;
		}
		b = 1;
		printf("Initializing... (%d/%d)\n\r", (r+1),K);
	}

}

float op_sin(int k, int b)
{
	int count = 0;
	while (b % 2 == 0)
	{
		b /= 2;
		count++;
	}
	if (sin_table != NULL)
	{
		return sin_table[k][count+1];
	}
}

float op_cos(int k, int b)
{
	int count = 0;
	while (b % 2 == 0)
	{
		b /= 2;
		count++;
	}
	if (cos_table != NULL)
	{
		return cos_table[k][count+1];
	}
}
