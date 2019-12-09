/*
 * project.c
 *
 * Main file
 *
 * Author: Peter Sutton. Modified by <YOUR NAME HERE>
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "ledmatrix.h"
#include "scrolling_char_display.h"
#include "buttons.h"
#include "serialio.h"
#include "terminalio.h"
#include "score.h"
#include "timer0.h"
#include "game.h"
#include "lives.h"

#define F_CPU 8000000L
#include <util/delay.h>

// Function prototypes - these are defined below (after main()) in the order
// given here
void initialise_hardware(void);
void splash_screen(void);
void new_game(void);
void play_game(void);
void handle_game_over(void);
void handle_win(void);

// ASCII code for Escape character
#define ESCAPE_CHAR 27

/////////////////////////////// main //////////////////////////////////
int main(void) {
	// Setup hardware and call backs. This will turn on 
	// interrupts.
	initialise_hardware();
	
	// Show the splash screen message. Returns when display
	// is complete
	splash_screen();
	
	while(1) {
		new_game();
		play_game();
		handle_game_over();
	}
}

void initialise_hardware(void) {
	ledmatrix_setup();
	init_button_interrupts();
	// Setup serial port for 19200 baud communication with no echo
	// of incoming characters
	init_serial_stdio(19200,0);
	
	init_timer0();

	// Turn on global interrupts
	sei();
}

void splash_screen(void) {
	// Clear terminal screen and output a message
	clear_terminal();
	move_cursor(10,10);
	printf_P(PSTR("Asteroids"));
	move_cursor(10,12);
	printf_P(PSTR("CSSE2010/7201 project by Ruidan Wang 45121063"));
	
	
	// Output the scrolling message to the LED matrix
	// and wait for a push button to be pushed.
	ledmatrix_clear();
	while(1) {
		set_scrolling_display_text("ASTEROIDS Ruidan Wang 45121063", COLOUR_GREEN);
		// Scroll the message until it has scrolled off the 
		// display or a button is pushed
		while(scroll_display()) {
			_delay_ms(150);
			if(button_pushed() != NO_BUTTON_PUSHED) {
				return;
			}
		}
	}
}

void new_game(void) {
	/* Set up the serial port for stdin communication at 19200 baud, no echo */
	init_serial_stdio(19200,0);
	
	/* Turn on global interrupts */
	sei();
	
	// Set up ADC - AVCC reference, right adjust
	// Input selection doesn't matter yet - we'll swap this around in the while
	// loop below.
	ADMUX = (1<<REFS0);
	
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1);
	
		
	// Initialise the game and display
	initialise_game();
	
	// Clear the serial terminal
	clear_terminal();
	
	// Initialise the score
	init_score();
	
	//Initialise the health bar
	init_lives();
	
	// Clear a button push or serial input if any are waiting
	// (The cast to void means the return value is ignored.)
	(void)button_pushed();
	clear_serial_input_buffer();
}

void play_game(void) {
	uint16_t value;
	uint8_t x_or_y = 0;	/* 0 = x, 1 = y */
	uint32_t current_time, last_move_time,falling_time,falling_last_time,left_time,left_over_time,right_time,right_over_time,hit_time,hit_over_time;
	int8_t button;
	char serial_input, escape_sequence_char;
	uint8_t characters_into_escape_sequence = 0;
	int8_t pause;
	
	// Get the current time and remember this as the last time the projectiles
    // were moved.
	current_time = get_current_time();
	last_move_time = current_time;
	falling_time = get_current_time();
	falling_last_time = falling_time;
	left_time = get_current_time();
	left_over_time = left_time;
	right_time = get_current_time();
	right_over_time = right_time;
	hit_time = get_current_time();
	hit_over_time = hit_time;
	pause = 0;
	
	// We play the game until it's over
	while(!is_game_over()) {
		
		if(x_or_y == 0) {
			ADMUX &= ~1;
			} else {
			ADMUX |= 1;
		}
		// Start the ADC conversion
		ADCSRA |= (1<<ADSC);
		
		while(ADCSRA & (1<<ADSC)) {
			; /* Wait until conversion finished */
		}
		value = ADC; // read the value
		if(x_or_y == 0) {
			if(value < 400){
				left_time = get_current_time();
				if(!is_game_over() && left_time >= left_over_time + 300 && pause == 0){
					move_base(MOVE_LEFT);
					left_over_time = left_time;
				}
				
			}else if(value > 600) {
				right_time = get_current_time();
				if(!is_game_over() && right_time >= right_over_time + 300 && pause == 0){
					move_base(MOVE_RIGHT);
					right_over_time = right_time;
				}
			}
		}else{
			if (value<400 || value >600)
			{fire_projectile();
			}
		}
		// Next time through the loop, do the other direction
		x_or_y ^= 1;
		
		// Check for input - which could be a button push or serial input.
		// Serial input may be part of an escape sequence, e.g. ESC [ D
		// is a left cursor key press. At most one of the following three
		// variables will be set to a value other than -1 if input is available.
		// (We don't initalise button to -1 since button_pushed() will return -1
		// if no button pushes are waiting to be returned.)
		// Button pushes take priority over serial input. If there are both then
		// we'll retrieve the serial input the next time through this loop
		serial_input = -1;
		escape_sequence_char = -1;
		button = button_pushed();
		
		if(button == NO_BUTTON_PUSHED ) {
			// No push button was pushed, see if there is any serial input
			if(serial_input_available()) {
				// Serial data was available - read the data from standard input
				serial_input = fgetc(stdin);
				// Check if the character is part of an escape sequence
				if(characters_into_escape_sequence == 0 && serial_input == ESCAPE_CHAR) {
					// We've hit the first character in an escape sequence (escape)
					characters_into_escape_sequence++;
					serial_input = -1; // Don't further process this character
				} else if(characters_into_escape_sequence == 1 && serial_input == '[') {
					// We've hit the second character in an escape sequence
					characters_into_escape_sequence++;
					serial_input = -1; // Don't further process this character
				} else if(characters_into_escape_sequence == 2) {
					// Third (and last) character in the escape sequence
					escape_sequence_char = serial_input;
					serial_input = -1;  // Don't further process this character - we
										// deal with it as part of the escape sequence
					characters_into_escape_sequence = 0;
				} else {
					// Character was not part of an escape sequence (or we received
					// an invalid second character in the sequence). We'll process 
					// the data in the serial_input variable.
					characters_into_escape_sequence = 0;
				}
			}
		}
		
		// Process the input. 
		
		if((button==3 || escape_sequence_char=='D' || serial_input=='L' || serial_input=='l')&& pause ==0) {
			// Button 3 pressed OR left cursor key escape sequence completed OR
			// letter L (lowercase or uppercase) pressed - attempt to move left
			move_base(MOVE_LEFT);
		} else if((button==2 || escape_sequence_char=='A' || serial_input==' ')&& pause ==0) {
			// Button 2 pressed or up cursor key escape sequence completed OR
			// space bar pressed - attempt to fire projectile
			fire_projectile();
		} else if((button==1 || escape_sequence_char=='B')&& pause ==0) {
			// Button 1 pressed OR down cursor key escape sequence completed
			// Ignore at present
			new_game();
		} else if((button==0 || escape_sequence_char=='C' || serial_input=='R' || serial_input=='r') && pause ==0) {
			// Button 0 pressed OR right cursor key escape sequence completed OR
			// letter R (lowercase or uppercase) pressed - attempt to move right
			move_base(MOVE_RIGHT);
		} else if(serial_input == 'p' || serial_input == 'P') {
			// Unimplemented feature - pause/unpause the game until 'p' or 'P' is
			// pressed again
			pause = 1 ^ pause;
		} 
		// else - invalid input or we're part way through an escape sequence -
		// do nothing
		hit_time = get_current_time();
		if(!is_game_over() && hit_time >= hit_over_time + 10 && pause == 0) {
			// 10ms has check projectiles hit asteroids and asteroids hit base station
			hit();
			hit_base();
			
			hit_over_time = current_time;
		}
		
		falling_time = get_current_time();
		if (get_score()<= 15){
			if(!is_game_over() && falling_time >= falling_last_time + 1500 && pause == 0) {
				// 2000ms (2 second) has passed since the last time we moved
				// the asteroids - move them - and keep track of the time we
				// moved them
				advance_asteroids();
			
				falling_last_time = current_time;
			}
		}else if (get_score() > 15 && get_score()<= 60){
			if(!is_game_over() && falling_time >= falling_last_time + 1000 && pause == 0) {
				// 1000ms (1 second) has passed since the last time we moved
				// the asteroids - move them - and keep track of the time we
				// moved them
				advance_asteroids();
				
				falling_last_time = current_time;
			}
		}else if (get_score() > 60){
			if(!is_game_over() && falling_time >= falling_last_time + 500 && pause == 0) {
				// 500ms (0.5 second) has passed since the last time we moved
				// the asteroids - move them - and keep track of the time we
				// moved them
				advance_asteroids();
				
				falling_last_time = current_time;
			}
		}

		current_time = get_current_time();
		if(!is_game_over() && current_time >= last_move_time + 500 && pause == 0) {
			// 500ms (0.5 second) has passed since the last time we moved
			// the projectiles - move them - and keep track of the time we 
			// moved them
			advance_projectiles();
			
			last_move_time = current_time;
		}
	}
	// We get here if the game is over.
	if (is_game_over() == 1){
		if (get_lives()==0)
		{handle_game_over();
		}else if (get_lives()>=99)
		{handle_win();
		}
	}
}

void handle_win(){
	move_cursor(10,14);
	printf_P(PSTR("Congratulation"));
	move_cursor(10,15);
	printf_P(PSTR("Press a button to start again"));
	
	ledmatrix_clear();
	while(1) {
		set_scrolling_display_text("Congratulation", COLOUR_GREEN);
		while(scroll_display()) {
			_delay_ms(150);
			if(button_pushed() != NO_BUTTON_PUSHED) {
				new_game();
				return;
			}
		}
	}
}

void handle_game_over() {
	move_cursor(10,14);
	printf_P(PSTR("GAME OVER"));
	move_cursor(10,15);
	printf_P(PSTR("Press a button to start again"));
	
	ledmatrix_clear();
	while(1) {
		set_scrolling_display_text("GAME OVER", COLOUR_LIGHT_ORANGE);
		while(scroll_display()) {
			_delay_ms(150);
			if(button_pushed() != NO_BUTTON_PUSHED) {
				new_game();
				return;
			}
		}
	}
	
}
