#include<stdio.h>
#include<stdlib.h>

int segment_data[5];
int dec_to_7seg[19]={
	0xC0,    //0
	0xF9,    //1
	0xA4,    //2
	0xB0,    //3
	0x99,    //4
	0x92,    //5
	0x82,    //6 
	0xF8,    //7
	0x80,    //8
	0x98,    //9
	0xFF,    //OFF
	0x7F};//0x07,           //colon? (11)



//***********************************************************************************
//                                   segment_sum                                    
//takes a 16-bit binary input value and places the appropriate equivalent 4 digit 
//BCD segment code in the array segment_data for display.                       
//array is loaded at exit as:  |digit3|digit2|colon|digit1|digit0|
void segsum(int sum) {
	//if(sum > 9999)
	//determine how many digits there are 
	//Fillint in backward
	int i;
	for( i=0; (sum/10) > 0 ; ++i){
		segment_data[i] = sum%10;
		sum /= 10;
	}
	if(sum != 0){
		segment_data[i] = sum;
		sum /= 10;
	}
	//Translate to 7seg
	segment_data[4] = (segment_data[4]);
	segment_data[3] = (segment_data[3]);
	segment_data[2] = (segment_data[2]);
	segment_data[1] = (segment_data[1]);
	segment_data[0] = (segment_data[0]);

	//TODO
	//blank out leadint zero digits 
	for(int j=i; j<5; ++j){
		if(segment_data[j]==0)
			segment_data[j] = 0x00;
	}

	//now move data to right place for misplaced colon position
}//segment_sum
//***********************************************************************************


//***********************************************************************************
int main()
{
	int sum=1234;
	segsum(sum);
	for(int i=0; i<5; ++i){
		printf("segment_data[%i] = %x\n",i,segment_data[i]);
		printf("i: %i",i<<2);
	}

	return 0;
}//main
