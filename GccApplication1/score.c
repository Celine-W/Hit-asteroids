/*
 * score.c
 *
 * Written by Peter Sutton
 */
#include "score.h"
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
uint32_t score;

/* Seven segment display digit being displayed.
** 0 = right digit; 1 = left digit.
*/
volatile uint8_t seven_seg_cc = 0;

/* Seven segment display segment values for 0 to 9 */
uint8_t seven_seg_data[10] = {63,6,91,79,102,109,125,7,127,111};

void init_score(void) {
	//display the score (to the serial terminal in a fixed position)
	move_cursor(50,10);
	printf_P(PSTR("Score:"));
	
	score = 0;
}

void add_to_score(uint16_t value) {
	score += value;
	//display the score (to the serial terminal in a fixed position) and update the score display only when it changes
	int8_t newscore = get_score();
	move_cursor(57,10);
	printf_P(PSTR("%d"), newscore);
}

uint32_t get_score(void) {
	return score;
}

void seven_seg(void){
	/* Make all bits of port A and the least significant
	** bit of port C be output bits.
	*/
	DDRC = 0xFF;
	DDRD = 0x04;
	
	/* Display a digit */
	if (get_score() < 10){
		/* Output the digit selection (CC) bit */
		PORTD = (0 << PIND2);
		/* Display rightmost digit - score which is less
		than 10. */
		PORTC = seven_seg_data[get_score()%10];
	}else{
		if(seven_seg_cc == 0) {
		/* Display rightmost digit - score which is less
		than 10. */
			PORTC = seven_seg_data[get_score()%10];
		} else {
		/* Display leftmost digit - score */
			PORTC = seven_seg_data[(get_score()/10)%10];
		}
		/* Change which digit will be displayed. If last time was
		** left, now display right. If last time was right, now 
		** display left.
		*/
		
		/* Output the digit selection (CC) bit */
		PORTD = (seven_seg_cc << PIND2);
		seven_seg_cc = 1 ^ seven_seg_cc;
	}
	


}
	

