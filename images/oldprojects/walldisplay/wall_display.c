#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>


//PORTA = left 1/2 red
//PORTJ = right 1/2 red
//PORTE = right 1/2 green
//PORTH = left 1/2 green
//PORTL = right 1/2 blue
//PORTC = left 1/2 blue

#define RED_LEFT PORTA
#define RED_RIGHT PORTJ
#define GREEN_LEFT PORTH
#define GREEN_RIGHT PORTE
#define BLUE_LEFT PORTC
#define BLUE_RIGHT PORTL

#define INITRED 8
#define INITGREEN 8
#define INITBLUE 5

//status LED - RGB
//OC1C = G
//OC1B = B
//OC1A = R

#define FALSE 0
#define TRUE 1

#define MODE_TIME_LIMIT 30

#define EEPROM_ADDRESS 0



//volatile unsigned int row = 0;
//volatile unsigned int col = 0;

//variables used in timer interrupt
volatile unsigned char irow = 0;
volatile unsigned char istage = 0;

enum colors {
	RL,
	RR,
	GL,
	GR,
	BL,
	BR
};

enum modes {
	RANDOM,
	TEST_COLORS,
	WHOLE,
	HORIZONTAL,
	VERTICAL,
	DIAGONAL1,
	DIAGONAL2,
	DIAGONAL3,
	DIAGONAL4,
	ARROW,
	ARROW_REVERSE,
	NIBBLES,
	PING_PONG,
	OUT,
	IN,
	SPIRAL_OUT,
	ZIG_ZAG,
	SPINNER,
	STOP
};


unsigned char matrix[8][16][6];

#define COLOR_HISTORY_SIZE 130

#define NIBBLES_SIZE 10



#define PULSE_CLOCK PORTK |=_BV(PK0); PORTK &= ~_BV(PK0)
#define SET_ROW_OUTPUT PORTK |=_BV(PK1)
#define CLEAR_ROW_OUTPUT PORTK &= ~_BV(PK1)

void set_status_led( unsigned int red, unsigned int green, unsigned int blue){
	//A = red
	//B = blue
	//C = green
	OCR1A = red;
	OCR1B = blue;
	OCR1C = green;

}

struct color {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
};

void set_led(unsigned char row, unsigned char col, unsigned char red, unsigned char green, unsigned char blue){
	unsigned char count=0;

	if(col>7){
		//right side
		col = col - 8;
		for(count=0;count<16;++count){
			if(red>count){
				//set a 1 in the outputs
				matrix[row][count][RR] |= _BV(col);
			}else{
				//set a 0 in the outputs
				matrix[row][count][RR] &= ~_BV(col);
			}
			if(green>count){
				//set a 1 in the outputs
				matrix[row][count][GR] |= _BV(col);
			}else{
				//set a 0 in the outputs
				matrix[row][count][GR] &= ~_BV(col);
			}
			if(blue>count){
				//set a 1 in the outputs
				matrix[row][count][BR] |= _BV(col);
			}else{
				//set a 0 in the outputs
				matrix[row][count][BR] &= ~_BV(col);
			}
		}
	}else{
		//left side
		for(count=0;count<16;++count){
			if(red>count){
				//set a 1 in the outputs
				matrix[row][count][RL] |= _BV(col);
			}else{
				//set a 0 in the outputs
				matrix[row][count][RL] &= ~_BV(col);
			}
			if(green>count){
				//set a 1 in the outputs
				matrix[row][count][GL] |= _BV(col);
			}else{
				//set a 0 in the outputs
				matrix[row][count][GL] &= ~_BV(col);
			}
			if(blue>count){
				//set a 1 in the outputs
				matrix[row][count][BL] |= _BV(col);
			}else{
				//set a 0 in the outputs
				matrix[row][count][BL] &= ~_BV(col);
			}
		}	

	}
}
struct color get_led(unsigned char row, unsigned char col){
	unsigned char count=0;
	struct color thecolor;
	thecolor.red=0;
	thecolor.green=0;
	thecolor.blue=0;
	if(col>7){
		//right side
		col = col - 8;
		for(count=0;count<16;++count){
			if(matrix[row][count][RR] & _BV(col)){
				thecolor.red=count;
			}
			if(matrix[row][count][GR] & _BV(col)){
				thecolor.green=count;
			}

			if(matrix[row][count][BR] & _BV(col)){
				thecolor.blue=count;
			}
		}
	}else{
		//left side
		for(count=0;count<16;++count){
			if(matrix[row][count][RL] & _BV(col)){
				thecolor.red=count;
			}
			if(matrix[row][count][GL] & _BV(col)){
				thecolor.green=count;
			}

			if(matrix[row][count][BL] & _BV(col)){
				thecolor.blue=count;
			}
		}	
	}
	return thecolor;
}




void max_led_brightness(unsigned char cutoff){
	unsigned char row=0;
	unsigned char col=0;
	struct color thecolor;
	for(row=0;row<8;row++){
		for(col=0;col<16;col++){
			thecolor = get_led(row,col);
			if(thecolor.red>cutoff){
				thecolor.red=cutoff;
			}
			if(thecolor.green>cutoff){
				thecolor.green=cutoff;
			}
			if(thecolor.blue>cutoff){
				thecolor.blue=cutoff;
			}
			set_led(row,col,thecolor.red,thecolor.green,thecolor.blue);

		}
	}
}
void clear_screen(){

	int row=0;
	int col=0;
	for(row=0;row<8;row++){
		for(col=0;col<16;col++){
			set_led(row,col,0,0,0);
		}
	}
}
			



void ioinit(void) {
	//set up the chip


	//DDR -> 1=output 0=input
	//PORT -> 1=pull up 0=no pull up (when input)
	//PORT -> 1=high 0=low (when output)


	//outputs for columns
	DDRA=0xFF;
	PORTA=0x00;
	DDRC=0xFF;
	PORTC=0x00;
	DDRE=0xFF;
	PORTE=0x00;
	DDRH=0xFF;
	PORTH=0x00;
	DDRJ=0xFF;
	PORTJ=0x00;
	DDRL=0xFF;
	PORTL=0x00;

	DDRK=0xFF;
	PORTK=0x00;


	//set up PORTB5-7 for output 11100000
	//set pull ups for programming lines 00011111
	DDRB=0xE0;
	PORTB=0x1F;

	//set up timer1 for pwm output, for status LED
	//WGM12 = 1
	//WGM10 = 1
	//COM1A1 = 1
	//COM1B1 = 1
	//COM1C1 = 1
	//CS12 = 1 (256 prescalar)
	TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(COM1C1) | _BV(WGM10);
	TCCR1B = _BV(WGM12) | _BV(CS12);
	//set initial values to 0
	OCR1A = 0x00;
	OCR1B = 0x00;
	OCR1C = 0x00;

	//set up timer3 for timing
	//set to CTC mode, clears to 0 on match with OCRnA
	//prescalar of 64
	//TCCR3B = (1<<WGM32)|(1<<CS31)|(1<<CS30);
	//prescalar of 1
	TCCR3B = (1<<WGM32)|(1<<CS30);

	//set initial top as 2000 (gives 125hz interrupt cycle)
	//OCR3A = 2000;
	//set initial top as 1000 (gives 125hz * 8 rows * 16 stages interrupt cycle)
	OCR3A = 1000;

	//turn on interrupt for matching OCR1A  
	TIMSK3 |= (1<<OCIE3A);

	//turn on timer 4 for changing patterns
	//set up to CTC mode, clears on match with OCRnA
	//prescaler of 1024 - 15625 ticks per second
	TCCR4B = (1<<WGM42)|(1<<CS42)|(1<<CS40);
	//this should give us 1 second per interrupt
	OCR4A = 15625;

	//enable the interrupt
	TIMSK4 |= (1<<OCIE4A);


	//turn on the watchdog timer
	wdt_enable(WDTO_4S);

	//enable interrupts - this is the global interrupt enable flag
	SREG=0x80;
}

//must be global because it is modified in interrupt
volatile unsigned char mode = WHOLE;
volatile unsigned char old_mode = WHOLE;
volatile unsigned char mode_flag = FALSE;
volatile unsigned char fade_flag = FALSE;

unsigned char random_mode(){
	return rand()%STOP;
}

int mode_countdown=0;
SIGNAL(SIG_OUTPUT_COMPARE4A){

	if(mode_countdown >= MODE_TIME_LIMIT){
		mode_countdown=0;
		//change mode
		old_mode=mode;
		while((old_mode==mode)||(mode==STOP)){
			mode=random_mode();
		}
		//set mode flag
		mode_flag=TRUE;
		fade_flag=TRUE;
	}else{
		mode_countdown++;
	}
}

SIGNAL(SIG_OUTPUT_COMPARE3A){

	if(irow==7){
		SET_ROW_OUTPUT;
		istage = (istage + 1) % 16;
	}
	irow = (irow + 1) % 8;
	//load matrix values
	RED_LEFT = matrix[irow][istage][RL];
	RED_RIGHT = matrix[irow][istage][RR];
	GREEN_LEFT = matrix[irow][istage][GL];
	GREEN_RIGHT = matrix[irow][istage][GR];
	BLUE_LEFT = matrix[irow][istage][BL];
	BLUE_RIGHT = matrix[irow][istage][BR];

	PULSE_CLOCK;
	CLEAR_ROW_OUTPUT;

}

/* reurns the analog input channel ch */
/*
   int readADC(unsigned char ch){
   ADMUX=((1<<REFS0)|(1<<ADLAR))+ch;
   ADCSR=(1<<ADEN)|(1<<ADSC)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
   while((ADCSR)& (1<<ADSC)){
// wait for the ADC to do it's stuff.
asm volatile ("nop");

}
return (ADCH);
}*/

unsigned char random_color(){

	return rand()%17;
}

int random_bump(){
	return rand()%3-1;
}
unsigned char nibbles_random(){
	return rand()%4;
}




unsigned char matrix[8][16][6];
unsigned char colors[16][8][3];


void change_colors(unsigned char circular_buffer_start, struct color * color_history){
	if(color_history[(circular_buffer_start+1)%COLOR_HISTORY_SIZE].red<=1){
		if(random_bump()==1){
			color_history[circular_buffer_start].red = 2;
		}else{
			color_history[circular_buffer_start].red = 1;
		}

	}else if(color_history[(circular_buffer_start+1)%COLOR_HISTORY_SIZE].red==16){
		if(random_bump()==-1){
			color_history[circular_buffer_start].red = 15;
		}else{
			color_history[circular_buffer_start].red = 16;
		}
	}else{
		color_history[circular_buffer_start].red = color_history[(circular_buffer_start+1)%COLOR_HISTORY_SIZE].red + random_bump();
	}
	if(color_history[(circular_buffer_start+1)%COLOR_HISTORY_SIZE].green<=1){
		if(random_bump()==1){
			color_history[circular_buffer_start].green = 2;
		}else{
			color_history[circular_buffer_start].green = 1;

		}
	}else if(color_history[(circular_buffer_start+1)%COLOR_HISTORY_SIZE].green==16){
		if(random_bump()==-1){
			color_history[circular_buffer_start].green = 15;
		}else{
			color_history[circular_buffer_start].green = 16;
		}
	}else{
		color_history[circular_buffer_start].green = color_history[(circular_buffer_start+1)%COLOR_HISTORY_SIZE].green + random_bump();
	}
	if(color_history[(circular_buffer_start+1)%COLOR_HISTORY_SIZE].blue<=1){
		if(random_bump()==1){
			color_history[circular_buffer_start].blue = 2;
		}
	}else if(color_history[(circular_buffer_start+1)%COLOR_HISTORY_SIZE].blue==16){
		if(random_bump()==-1){
			color_history[circular_buffer_start].blue = 15;
		}else{
			color_history[circular_buffer_start].blue = 16;
		}
	}else{
		color_history[circular_buffer_start].blue = color_history[(circular_buffer_start+1)%COLOR_HISTORY_SIZE].blue + random_bump();
	}

}

void reset_color_history(struct color * color_history){
	unsigned char counter=0;
	for(counter = 0; counter<COLOR_HISTORY_SIZE;counter++){
		color_history[counter].red=INITRED;
		color_history[counter].green=INITGREEN;
		color_history[counter].blue=INITBLUE;
	}
}



int main(void)
{

	//int y=55;
	//int z=55;
	unsigned char stage=0;
	unsigned char stage_flip=0;
	unsigned char counter = 0;


	unsigned char row=0;
	unsigned char col=0;

	struct color color_history[COLOR_HISTORY_SIZE];


	unsigned char circular_buffer_start=0;

	//used in TEST_COLORS
	unsigned char test_colors=0;

	//used in NIBBLES
	unsigned char xs[NIBBLES_SIZE];
	unsigned char ys[NIBBLES_SIZE];
	unsigned char pass=FALSE;

	//used in PING_PONG
	unsigned char ball_x=3;
	unsigned char ball_y=3;
	unsigned char left=5;
	unsigned char right=3;
	unsigned char ball_dir=1;

	//used by SPIRAL modes to calculate offsets
	unsigned char offset=0;

	//used by spinner
	unsigned char spinner_offset[8][16];
	unsigned char spinner_flag=FALSE;


	//values for eeprom reading/writing
	
	int eeprom_value=0;

	//get value from eeprom for random seed
	eeprom_value = eeprom_read_word(EEPROM_ADDRESS);
	//seed random with this value
	srand(eeprom_value);
	//take first random number and put it as a seed for next time
	eeprom_write_word(EEPROM_ADDRESS,rand());
	//read and update eeprom count
	eeprom_value = eeprom_read_word(EEPROM_ADDRESS + 2);
	eeprom_write_word(EEPROM_ADDRESS + 2, eeprom_value + 1);

	mode = random_mode();
	//mode = NIBBLES;




	reset_color_history(color_history);

	//init matrix values
	/*
	   for(y=0;y<8;y++){
	   for(z=0;z<16;z++){
	   matrix[y][z][0]=0xFF;
	   matrix[y][z][1]=0xFF;
	   matrix[y][z][2]=0xFF;
	   matrix[y][z][3]=0xFF;
	   matrix[y][z][4]=0xFF;
	   matrix[y][z][5]=0xFF;
	   }
	   }*/

	//initialize nibbles values
	for(counter = 0; counter < NIBBLES_SIZE; counter++){
		xs[counter]=0;
		ys[counter]=0;
	}


	ioinit(); //this inits all the important ports

	for(;;){ //our main loop - inf loop
		//reset watchdog timer
		wdt_reset();





		if(fade_flag){
			//fade all LEDS to off for mode switch
			for(counter=16;counter>0;counter--){

				max_led_brightness(counter-1);
				_delay_ms(100.0);
				wdt_reset();
			}
			fade_flag=FALSE;
		}

			


		switch(mode){
			case RANDOM:
				for(row=0;row<8;row++){
					for(col=0;col<16;col++){
						set_led(row,col,random_color(),random_color(),random_color());
					}
				}
				break;
			case TEST_COLORS:
				test_colors = (test_colors + 1)%102;
				for(row=0;row<8;row++){
					for(col=0;col<16;col++){
						if(test_colors<17){
							set_led(row,col,test_colors,0,0);
						}else if(test_colors<34){
							set_led(row,col,33-test_colors,0,0);
						}else if(test_colors<51){
							set_led(row,col,0,test_colors%17,0);
						}else if(test_colors<68){
							set_led(row,col,0,(67-test_colors),0);
						}else if(test_colors<85){
							set_led(row,col,0,0,test_colors%17);
						}else{
							set_led(row,col,0,0,(101-test_colors));
						}
					}
				}
				_delay_ms(200.0);
				break;
			case WHOLE:

				switch(random_bump()){
					case -1:
						if(color_history[0].red<1){
							if(random_bump()==1){
								color_history[0].red++;
							}
						}else if(color_history[0].red==16){
							if(random_bump()==-1){
								color_history[0].red--;
							}
						}else{
							color_history[0].red += random_bump();
						}
						break;
					case 0:
						if(color_history[0].green<1){
							if(random_bump()==1){
								color_history[0].green++;
							}
						}else if(color_history[0].green==16){
							if(random_bump()==-1){
								color_history[0].green--;
							}
						}else{
							color_history[0].green += random_bump();
						}
						break;
					case 1:
						if(color_history[0].blue<1){
							if(random_bump()==1){
								color_history[0].blue++;
							}
						}else if(color_history[0].blue==16){
							if(random_bump()==-1){
								color_history[0].blue--;
							}
						}else{
							color_history[0].blue += random_bump();
						}
						break;
				}

				for(row=0;row<8;row++){
					for(col=0;col<16;col++){
						set_led(row,col,color_history[0].red,color_history[0].green,color_history[0].blue);
					}
				}
				_delay_ms(100.0);
				_delay_ms(100.0);
				_delay_ms(100.0);
				break;

			case HORIZONTAL:
				if(mode_flag){
					mode_flag=FALSE;
					reset_color_history(color_history);
				}
				//for this mode we keep 16 colors
				//generate a new one
				if(circular_buffer_start==0){
					circular_buffer_start = COLOR_HISTORY_SIZE - 1;
				}else{
					circular_buffer_start--;
				}
				change_colors(circular_buffer_start, color_history);

				for(col=0;col<16;col++){
					for(row=0;row<8;row++){
						set_led(row,col,color_history[(circular_buffer_start+col)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+col)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+col)%COLOR_HISTORY_SIZE].blue);
					}
				}
				_delay_ms(100.0);
				break;
			case VERTICAL:
				if(mode_flag){
					mode_flag=FALSE;
					reset_color_history(color_history);
				}
				//for this mode we keep 8 colors
				//generate a new one
				if(circular_buffer_start==0){
					circular_buffer_start = COLOR_HISTORY_SIZE - 1;
				}else{
					circular_buffer_start--;
				}
				change_colors(circular_buffer_start, color_history);

				for(row=0;row<8;row++){
					for(col=0;col<16;col++){
						set_led(row,col,color_history[(circular_buffer_start+row)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+row)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+row)%COLOR_HISTORY_SIZE].blue);
					}
				}
				_delay_ms(100.0);
				break;
			case DIAGONAL1:
				if(mode_flag){
					mode_flag=FALSE;
					reset_color_history(color_history);
				}
				//for this mode we keep 22 colors
				//generate a new one
				if(circular_buffer_start==0){
					circular_buffer_start = COLOR_HISTORY_SIZE - 1;
				}else{
					circular_buffer_start--;
				}
				change_colors(circular_buffer_start, color_history);

				for(row=0;row<8;row++){
					for(col=0;col<16;col++){
						set_led(row,col,color_history[(circular_buffer_start+row+col)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+row+col)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+row+col)%COLOR_HISTORY_SIZE].blue);
					}
				}
				_delay_ms(100.0);
				break;

			case DIAGONAL2:
				if(mode_flag){
					mode_flag=FALSE;
					reset_color_history(color_history);
				}
				//for this mode we keep 22 colors
				//generate a new one
				if(circular_buffer_start==0){
					circular_buffer_start = COLOR_HISTORY_SIZE - 1;
				}else{
					circular_buffer_start--;
				}

				change_colors(circular_buffer_start, color_history);

				for(row=0;row<8;row++){
					for(col=0;col<16;col++){
						set_led(row,col,color_history[(circular_buffer_start+22-row-col)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+22-row-col)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+22-row-col)%COLOR_HISTORY_SIZE].blue);
					}
				}
				_delay_ms(100.0);
				break;
			case DIAGONAL3:
				if(mode_flag){
					mode_flag=FALSE;
					reset_color_history(color_history);
				}
				//for this mode we keep 22 colors
				//generate a new one
				if(circular_buffer_start==0){
					circular_buffer_start = COLOR_HISTORY_SIZE - 1;
				}else{
					circular_buffer_start--;
				}
				change_colors(circular_buffer_start, color_history);

				for(row=0;row<8;row++){
					for(col=0;col<16;col++){
						set_led(row,col,color_history[(circular_buffer_start+15+row-col)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+15+row-col)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+15+row-col)%COLOR_HISTORY_SIZE].blue);
					}
				}
				_delay_ms(100.0);
				break;
			case DIAGONAL4:
				if(mode_flag){
					mode_flag=FALSE;
					reset_color_history(color_history);
				}
				//for this mode we keep 22 colors
				//generate a new one
				if(circular_buffer_start==0){
					circular_buffer_start = COLOR_HISTORY_SIZE - 1;
				}else{
					circular_buffer_start--;
				}
				change_colors(circular_buffer_start, color_history);

				for(row=0;row<8;row++){
					for(col=0;col<16;col++){
						set_led(row,col,color_history[(circular_buffer_start+7-row+col)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+7-row+col)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+7-row+col)%COLOR_HISTORY_SIZE].blue);
					}
				}
				_delay_ms(100.0);
				break;
			case ARROW:
				if(mode_flag){
					mode_flag=FALSE;
					reset_color_history(color_history);
				}
				//for this mode we keep 16 colors
				//generate a new one
				if(circular_buffer_start==0){
					circular_buffer_start = COLOR_HISTORY_SIZE - 1;
				}else{
					circular_buffer_start--;
				}
				change_colors(circular_buffer_start, color_history);

				for(row=0;row<8;row++){
					for(col=0;col<16;col++){
						if(row<=2){
							set_led(row,col,color_history[(circular_buffer_start+3-row+col)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+3-row+col)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+3-row+col)%COLOR_HISTORY_SIZE].blue);

						}else if(row>=5){
							set_led(row,col,color_history[(circular_buffer_start+row+col-4)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+row+col-4)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+row+col-4)%COLOR_HISTORY_SIZE].blue);

						}else{
							set_led(row,col,color_history[(circular_buffer_start+col)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+col)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+col)%COLOR_HISTORY_SIZE].blue);

						}
					}
				}
				_delay_ms(100.0);
				break;
			case ARROW_REVERSE:
				if(mode_flag){
					mode_flag=FALSE;
					reset_color_history(color_history);
				}
				//for this mode we keep 16 colors
				//generate a new one
				if(circular_buffer_start==0){
					circular_buffer_start = COLOR_HISTORY_SIZE - 1;
				}else{
					circular_buffer_start--;
				}
				change_colors(circular_buffer_start, color_history);

				for(row=0;row<8;row++){
					for(col=0;col<16;col++){
						if(row<=2){
							set_led(row,col,color_history[(circular_buffer_start+18-row-col)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+18-row-col)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+18-row-col)%COLOR_HISTORY_SIZE].blue);

						}else if(row>=5){
							set_led(row,col,color_history[(circular_buffer_start+11+row-col)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+11+row-col)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+11+row-col)%COLOR_HISTORY_SIZE].blue);

						}else{
							set_led(row,col,color_history[(circular_buffer_start+15-col)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+15-col)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+15-col)%COLOR_HISTORY_SIZE].blue);

						}
					}
				}
				_delay_ms(100.0);
				break;

			case OUT:
				if(mode_flag){
					mode_flag=FALSE;
					reset_color_history(color_history);
				}
				//for this mode we keep 11 colors
				//generate a new one
				if(circular_buffer_start==0){
					circular_buffer_start = COLOR_HISTORY_SIZE - 1;
				}else{
					circular_buffer_start--;
				}
				change_colors(circular_buffer_start, color_history);

				for(row=0;row<8;row++){
					for(col=0;col<8;col++){
						if(row<=2){
							set_led(row,col,color_history[(circular_buffer_start+10-row-col)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+10-row-col)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+10-row-col)%COLOR_HISTORY_SIZE].blue);

						}else if(row>=5){
							set_led(row,col,color_history[(circular_buffer_start+3+row-col)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+3+row-col)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+3+row-col)%COLOR_HISTORY_SIZE].blue);

						}else{
							set_led(row,col,color_history[(circular_buffer_start+7-col)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+7-col)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+7-col)%COLOR_HISTORY_SIZE].blue);

						}
					}
					for(col=8;col<16;col++){
						if(row<=2){
							set_led(row,col,color_history[(circular_buffer_start-5-row+col)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start-5-row+col)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start-5-row+col)%COLOR_HISTORY_SIZE].blue);

						}else if(row>=5){
							set_led(row,col,color_history[(circular_buffer_start+row+col-12)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+row+col-12)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+row+col-12)%COLOR_HISTORY_SIZE].blue);

						}else{
							set_led(row,col,color_history[(circular_buffer_start+col-8)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+col-8)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+col-8)%COLOR_HISTORY_SIZE].blue);

						}
					}
				}
				_delay_ms(100.0);
				break;

			case IN:
				if(mode_flag){
					mode_flag=FALSE;
					reset_color_history(color_history);
				}
				//for this mode we keep 11 colors
				//generate a new one
				if(circular_buffer_start==0){
					circular_buffer_start = COLOR_HISTORY_SIZE - 1;
				}else{
					circular_buffer_start--;
				}
				change_colors(circular_buffer_start, color_history);

				for(row=0;row<8;row++){
					for(col=8;col<16;col++){
						if(row<=2){
							set_led(row,col,color_history[(circular_buffer_start+15+row-col)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+15+row-col)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+15+row-col)%COLOR_HISTORY_SIZE].blue);

						}else if(row>=5){
							set_led(row,col,color_history[(circular_buffer_start+22-row-col)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+22-row-col)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+22-row-col)%COLOR_HISTORY_SIZE].blue);

						}else{
							set_led(row,col,color_history[(circular_buffer_start+18-col)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+18-col)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+18-col)%COLOR_HISTORY_SIZE].blue);

						}
					}
					for(col=0;col<8;col++){
						if(row<=2){
							set_led(row,col,color_history[(circular_buffer_start+row+col)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+row+col)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+row+col)%COLOR_HISTORY_SIZE].blue);

						}else if(row>=5){
							set_led(row,col,color_history[(circular_buffer_start-row+col+7)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start-row+col+7)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start-row+col+7)%COLOR_HISTORY_SIZE].blue);

						}else{
							set_led(row,col,color_history[(circular_buffer_start+col+3)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+col+3)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+col+3)%COLOR_HISTORY_SIZE].blue);

						}
					}
				}
				_delay_ms(100.0);
				break;
			case SPIRAL_OUT:
				if(mode_flag){
					mode_flag=FALSE;
					reset_color_history(color_history);
				}
				//for this mode we keep 125 colors
				//generate a new one
				if(circular_buffer_start==0){
					circular_buffer_start = COLOR_HISTORY_SIZE - 1;
				}else{
					circular_buffer_start--;
				}
				change_colors(circular_buffer_start, color_history);
				for(row=0;row<8;row++){
					for(col=0;col<4;col++){
						offset = (3 - col) * 16 + 61 + row;
						set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
					}
					for(col=12;col<16;col++){
						offset = (col - 12) * 16 + 76 - row;
						set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
					}
					col=11;
					offset = 53 - row;
					set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);

				}
				for(col=4;col<11;col++){
					row=0;
					offset = 64 - col;
					set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
					row=1;
					offset = 37 - col;
					set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
					row=7;
					offset = 35 + col;
					set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);

				}

				for(row=2;row<7;row++){
					col=4;
					offset = 32 + row;
					set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
					col=5;
					offset = 11 + row;
					set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
					col=10;
					offset = 28 - row;
					set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
				}

				for(col=6;col<10;col++){
					row=2;
					offset = 18 - col;
					set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
					row=6;
					offset = 12 + col;
					set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);

				}
				row=3;
				col=6;
				offset=1;
				set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
				col=9;
				offset=8;
				set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
				col=8;
				offset=0;
				set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
				col=7;
				set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
				row=4;
				col=6;
				offset=2;
				set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
				col=9;
				offset=7;
				set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
				col=7;
				offset=0;
				set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
				col=8;
				set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
				row=5;
				col=6;
				offset=3;
				set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
				col=7;
				offset=4;
				set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
				col=8;
				offset=5;
				set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);
				col=9;
				offset=6;
				set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);




				_delay_ms(100.0);






				break;

			case ZIG_ZAG:
				if(mode_flag){
					mode_flag=FALSE;
					reset_color_history(color_history);
				}
				//for this mode we keep 128 colors
				//generate a new one
				if(circular_buffer_start==0){
					circular_buffer_start = COLOR_HISTORY_SIZE - 1;
				}else{
					circular_buffer_start--;
				}
				change_colors(circular_buffer_start, color_history);

				for(row=0;row<8;row++){
					for(col=0;col<16;col++){

						if(col%2){
							offset = col * 8 + 7 - row;
						}else{
							offset = col * 8 + row;
						}
						set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);

					}
				}
				_delay_ms(100.0);


				break;

			case SPINNER:
				if(mode_flag){
					mode_flag=FALSE;
					reset_color_history(color_history);
				}
				//for this mode we keep 5 colors
				//generate a new one
				if(circular_buffer_start==0){
					circular_buffer_start = COLOR_HISTORY_SIZE - 1;
				}else{
					circular_buffer_start--;
				}
				change_colors(circular_buffer_start, color_history);

				if(!spinner_flag){
					//init offset array
					spinner_offset[0][0]=4;
					spinner_offset[1][0]=4;
					spinner_offset[0][6]=4;
					spinner_offset[0][7]=4;
					spinner_offset[7][0]=4;
					spinner_offset[7][1]=4;
					spinner_offset[6][7]=4;
					spinner_offset[7][7]=4;
					
					spinner_offset[2][0]=3;
					spinner_offset[3][0]=3;
					spinner_offset[3][1]=3;
					spinner_offset[0][4]=3;
					spinner_offset[0][5]=3;
					spinner_offset[1][4]=3;
					spinner_offset[4][6]=3;
					spinner_offset[4][7]=3;
					spinner_offset[5][7]=3;
					spinner_offset[6][3]=3;
					spinner_offset[7][3]=3;
					spinner_offset[7][2]=3;
					
					spinner_offset[4][0]=2;
					spinner_offset[4][1]=2;
					spinner_offset[4][2]=2;
					spinner_offset[3][7]=2;
					spinner_offset[3][6]=2;
					spinner_offset[3][5]=2;
					spinner_offset[0][3]=2;
					spinner_offset[1][3]=2;
					spinner_offset[2][3]=2;
					spinner_offset[5][4]=2;
					spinner_offset[6][4]=2;
					spinner_offset[7][4]=2;

					spinner_offset[5][0]=1;
					spinner_offset[5][1]=1;
					spinner_offset[5][2]=1;
					spinner_offset[2][7]=1;
					spinner_offset[2][6]=1;
					spinner_offset[2][5]=1;
					spinner_offset[0][2]=1;
					spinner_offset[1][2]=1;
					spinner_offset[2][2]=1;
					spinner_offset[5][5]=1;
					spinner_offset[6][5]=1;
					spinner_offset[7][5]=1;
					
					spinner_offset[0][1]=0;
					spinner_offset[1][1]=0;
					spinner_offset[2][1]=0;
					spinner_offset[5][6]=0;
					spinner_offset[6][6]=0;
					spinner_offset[7][6]=0;
					spinner_offset[6][0]=0;
					spinner_offset[6][1]=0;
					spinner_offset[6][2]=0;
					spinner_offset[1][5]=0;
					spinner_offset[1][6]=0;
					spinner_offset[1][7]=0;
					
					spinner_offset[3][2]=0;
					spinner_offset[3][3]=0;
					spinner_offset[3][4]=0;
					spinner_offset[4][3]=0;
					spinner_offset[4][4]=0;
					spinner_offset[4][5]=0;
					spinner_offset[5][3]=0;
					spinner_offset[2][4]=0;
					for(row=0;row<8;row++){
						for(col=8;col<16;col++){
							spinner_offset[row][col]=spinner_offset[row][col-8] + 25;
						}
					}

					spinner_flag = TRUE;

				}

				for(row=0;row<8;row++){
					for(col=0;col<16;col++){
						offset = spinner_offset[row][col];
						set_led(row,col,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].red,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].green,color_history[(circular_buffer_start+offset)%COLOR_HISTORY_SIZE].blue);

					}
				}
				_delay_ms(100.0);


				break;
			case NIBBLES:
				//check mode_flag, clear screen if it is set
				/*if(mode_flag){
					clear_screen();
					mode_flag=FALSE;
				}*/

				//turn off last one
				set_led(ys[NIBBLES_SIZE-1],xs[NIBBLES_SIZE-1],0,0,0);


				//move back positions
				for(counter=NIBBLES_SIZE-1;counter>0;counter--){
					xs[counter]=xs[counter-1];
					ys[counter]=ys[counter-1];
				}
				//pick a direction that is legal (not into wall, not back to same position as before)
				pass=FALSE;
				while(!pass){
					switch(nibbles_random()){
						case 0:
							//up
							if((ys[1]-1 != ys[2]) && (ys[1] != 0)){
								ys[0]=ys[1]-1;
								pass=TRUE;
							}
							break;
						case 1:
							//down
							if((ys[1]+1 != ys[2]) && (ys[1] != 7)){
								ys[0]=ys[1]+1;
								pass=TRUE;
							}

							break;
						case 2:
							//left
							if((xs[1]-1 != xs[2]) && (xs[1] != 0)){
								xs[0]=xs[1]-1;
								pass=TRUE;
							}

							break;
						case 3:
							//right
							if((xs[1]+1 != xs[2]) && (xs[1] != 15)){
								xs[0]=xs[1]+1;
								pass=TRUE;
							}

							break;
					}
				}


				//pick a new color
				switch(random_bump()){
					case -1:
						if(color_history[0].red<=1){
							if(random_bump()==1){
								color_history[0].red++;
							}
						}else if(color_history[0].red==16){
							if(random_bump()==-1){
								color_history[0].red--;
							}
						}else{
							color_history[0].red += random_bump();
						}
						break;
					case 0:
						if(color_history[0].green<=1){
							if(random_bump()==1){
								color_history[0].green++;
							}
						}else if(color_history[0].green==16){
							if(random_bump()==-1){
								color_history[0].green--;
							}
						}else{
							color_history[0].green += random_bump();
						}
						break;
					case 1:
						if(color_history[0].blue<=1){
							if(random_bump()==1){
								color_history[0].blue++;
							}
						}else if(color_history[0].blue==16){
							if(random_bump()==-1){
								color_history[0].blue--;
							}
						}else{
							color_history[0].blue += random_bump();
						}
						break;
				}

				/*
				//show what color is being made on bottom rows
				for(counter=0;counter<16;counter++){
					if(counter<=color_history[0].red){
						set_led(5,counter,16,0,0);
					}else{
						set_led(5,counter,0,0,0);
					}

					if(counter<=color_history[0].green){
						set_led(6,counter,0,16,0);
					}else{
						set_led(6,counter,0,0,0);

					}
					if(counter<=color_history[0].blue){
						set_led(7,counter,0,0,16);
					}else{
						set_led(7,counter,0,0,0);

					}
				}
				*/


				for(counter=0;counter<NIBBLES_SIZE;counter++){
					set_led(ys[counter],xs[counter],color_history[0].red,color_history[0].green,color_history[0].blue);
				}

				/*
				for(row=0;row<8;row++){
					for(col=0;col<16;col++){
						for(counter=0;counter<NIBBLES_SIZE;counter++){
							if(col==xs[counter] && row==ys[counter]){
								//set_led(row,col,color_history[0].red,color_history[0].green,color_history[0].blue);
								set_led(row,col,color_history[0].red,color_history[0].green,0);
								break;
							}else{
								//turn this one off
								set_led(row,col,0,0,0);

							}
						}
					}
				}*/

				_delay_ms(100.0);

				break;
			case PING_PONG:
				//check mode_flag, clear screen if it is set
				if(mode_flag){
					//clear_screen();
					mode_flag=FALSE;
					ball_x=3;
					ball_y=3;
					left=5;
					right=3;
					ball_dir=1;
				}

				//clear last ball
				set_led(ball_y,ball_x,0,0,0);

				//move paddles
				//if((ball_x<8) && (ball_dir == 1 || ball_dir == 0)){
				//if((ball_x<8)){
				if((ball_dir <2)){
					if(left-ball_y > 0){
						if(left<6){
							left += 1;
						}
					}else if(left-ball_y < 0){
						if(left>1){
							left -= 1;
						}
					}
				}else{
					if(random_bump()==1){
						switch(random_bump()){
							case -1:
								if(left>1){
									left--;
								}
								break;
							case 1:
								if(left<6){
									left++;
								}
								break;
						}
					}

				}
				//if((ball_x>7) && (ball_dir > 1)){
				if((ball_dir > 1)){
					if(right-ball_y > 0){
						if(right<6){
							right += 1;
						}
					}else if(right-ball_y < 0){
						if(right>1){
							right -= 1;
						}
					}
				}else{
					if(random_bump()==1){
						switch(random_bump()){
							case -1:
								if(right>1){
									right--;
								}
								break;
							case 1:
								if(right<6){
									right++;
								}
								break;
						}
					}
				}

				switch(ball_x){
					case 1:
						//bounce to right
						if(ball_dir==0){
							ball_dir=2;
						}else{
							ball_dir=3;
						}
						break;
					case 14:
						//bounce to left
						if(ball_dir==2){
							ball_dir=0;
						}else{
							ball_dir=1;
						}
				}
				switch(ball_y){
					case 0:
						//bounce down
						if(ball_dir==1){
							ball_dir=0;
						}else{
							ball_dir=2;
						}
						break;
					case 7:
						//bounce up
						if(ball_dir==2){
							ball_dir=3;
						}else{
							ball_dir=1;
						}
				}
				switch(ball_dir){
					case 0:
						ball_y++;
						ball_x--;
						break;
					case 1:
						ball_y--;
						ball_x--;
						break;
					case 2:
						ball_y++;
						ball_x++;
						break;
					case 3:
						ball_y--;
						ball_x++;
						break;
				}


				//color the screen
				//left paddle
				for(row=0;row<8;row++){
					if((row==left-1)||(row==left)||(row==left+1)){
						set_led(row,0,0,255,0);
					}else{
						set_led(row,0,0,0,0);
					}
				}
				//right paddle
				for(row=0;row<8;row++){
					if((row==right-1)||(row==right)||(row==right+1)){
						set_led(row,15,0,0,255);
					}else{
						set_led(row,15,0,0,0);
					}
				}
				//ball
				set_led(ball_y,ball_x,255,0,0);

				_delay_ms(100.0);
				_delay_ms(100.0);
				_delay_ms(100.0);
				break;


		}


		//blink status LED the number of times for mode
		if(stage==STOP){
			stage=0;
		}else{
			if(mode>=stage){
				if(stage_flip){
					set_status_led(0,0,0);
					stage++;
					stage_flip=FALSE;
				}else{
					stage_flip=TRUE;
					set_status_led(0,25,0);
				}
			}else{
				stage++;
				set_status_led(0,0,0);
			}
		}

		/*
		switch(stage){
			case 0:
				set_status_led(255,0,0);
				stage++;
				break;
			case 1:
				set_status_led(0,255,0);
				stage++;
				break;
			case 2:
				set_status_led(0,0,255);
				stage=0;
				break;
			default:
				stage = 0;
				break;	
		}
		*/
		//_delay_ms(250.0);



	} //end for loop
}//end main
