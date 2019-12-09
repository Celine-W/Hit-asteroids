/*
 * lives.c
 *
 * Created: 2019/5/19 星期日 下午 3:05:03
 *  Author: Ruidan Wang
 */ 
#include "lives.h"
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
uint32_t lives;

void init_lives(void){
	DDRA = (1<<PIND3) | (1<<PIND4) | (1<<PIND5) | (1<<PIND6);
	PORTA |= (1<<PIND3) | (1<<PIND4) | (1<<PIND5) | (1<<PIND6);
	move_cursor(50,20);
	printf_P(PSTR("Lives: "));
	
	lives = 4;
	
	move_cursor(57,20);
	printf_P(PSTR("%d"),get_lives());
}

void reduce_to_lives(uint16_t value) {
	lives = lives - value;
	
	move_cursor(57,20);
	printf_P(PSTR("%d"),get_lives());
	
	if (lives==3){
		PORTA = (0<<PIND3) | (1<<PIND4) | (1<<PIND5) | (1<<PIND6);
	}
	if (lives == 2){
		PORTA = (0<<PIND3) | (1<<PIND4) | (1<<PIND5) | (0<<PIND6);
	}
	if (lives == 1){
		PORTA = (0<<PIND3) | (0<<PIND4) | (1<<PIND5) | (0<<PIND6);
	} 
	if (lives == 0){
		PORTA = 0;
	}
}

uint32_t get_lives(void) {
	return lives;
}