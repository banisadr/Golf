/************************************************************
Header Information
************************************************************/

/*
 * Golf.c
 *
 * Created: 10/22/2015 6:10:at42 PM
 * Authors: Wyatt Shapiro, Bahram Banisadr
 * MEAM 510
 * Lab 5: Golf
 */ 

/************************************************************
Included Files & Libraries
************************************************************/

#include <avr/io.h>
#include "m_general.h"
#include "m_usb.h"

/************************************************************
Definitions
************************************************************/

#define TIME_CONSTANT 4.4 // Seconds
#define RESISTANCE 1000 // Ohms
#define CAPACITANCE 4400 // uF
#define SYSTEM_SPEED 2000000 // Hz
#define TIMER_ONE_PRESCALER 1024 // No Units
#define TIME_CONSTANTS_TO_MAX 5 // Number of time constants until ~100% charge
#define PHOTOTRANSISTOR_ADJUSTMENT 1

/************************************************************
Prototype Functions
************************************************************/

// Enabling system functions
void input_enable(void); // enable input pins 
void output_enable(void); // enable output pins
void ADC_enable(void); // enable ADC routine
void timer1_enable(void); // enable timer routine
void usb_enable(void);

// Initializations
void output_init(void); // initialize output pins
void ADC_init(void); // initialize ADC routine 

/************************************************************
Global Variables
************************************************************/

int firing_ready = 0;
float ocr1a_constant = PHOTOTRANSISTOR_ADJUSTMENT*SYSTEM_SPEED/
	TIMER_ONE_PRESCALER*TIME_CONSTANT*TIME_CONSTANTS_TO_MAX; // Used to calculate OCR1A value in ADC intterupt

/************************************************************
Main Loop
************************************************************/

int main(void)
{

	m_red(ON); // Confirm Power
	input_enable();
	output_enable();
	ADC_enable();
	timer1_enable();
	m_green(ON); // Confirm Initializations
	
	output_init();
	ADC_init();
	
	
	
    while(1)
    {
		m_usb_tx_int(firing_ready);
		// When firing is ready 
		if(firing_ready){

			cli(); // Dissable global interrupts
			
			set(PORTB,5); // turn on N channel 2 pin (FIREEEEE)
			
			firing_ready = 0; // turn off firing ready flag

			m_wait(10000); // wait 2 seconds for complete discharge

			set(PORTD,0); // turn off green light
			set(PORTD,3); // turn off red light
			

			
			output_init(); // restart initialization
			//ADC_init(); // Must restart M2 to begin charging again

			// When firing switch is on
			/*
	        if (!check(PORTB,2)){

				m_usb_tx_string("\n FIREEEEEE");
				
				set(PORTB,5); // turn on N channel 2 pin (FIREEEEE)
				
				firing_ready = 0; // turn off firing ready flag

				set(PORTD,0); // turn off green light
				set(PORTD,3); // turn off red light
				
				m_wait(250); // wait 2 seconds for complete discharge
				
				output_init(); // restart initialization
				//ADC_init(); // Must restart M2 to begin charging again
			
			}*/
        }
    }
}


/************************************************************
Initialization of Subsystem Components
************************************************************/

// Setup USB
void usb_enable(void)
{
	m_usb_init();
	while(!m_usb_isconnected());
}

// Set Input Pins
void input_enable(void)
{
	clear(DDRD,7); // Pin D7 - photo transistor ADC input
	clear(DDRB,2); // Pin B2 - firing switch input
	
	set(PORTB,2); // Pin B2 - enable pull up resistors on firing switch
} 

// Set Output Pins
void output_enable(void)
{
	set(DDRB,4); // Pin B4 - N channel 1 output
	set(DDRB,5); // Pin B5 - N channel 2 output
	set(DDRD,0); // Pin D0 - Green LED output
	set(DDRD,3); // Pin D3 - Red LED 2 output
}

// Enable, but do not start ADC
void ADC_enable(void)
{
	m_clockdivide(3); // reduces clock speed to 2MHz
	
	// set analog conversion
	
	clear(ADMUX,REFS1); // Set voltage reference to 5V
	set(ADMUX,REFS0); // ^

	clear(ADCSRA,ADPS2); // Scale ADC /8 to 250kHz
	set(ADCSRA,ADPS1); // ^
	set(ADCSRA,ADPS0); // ^

	set(DIDR0,ADC7D); // disable 7 pin's digital input

	clear(ADCSRA,ADATE); // clear free-running mode

	set(ADCSRB,MUX5);// make input D7
	clear(ADMUX,MUX2); // ^
	set(ADMUX,MUX1); // ^
	clear(ADMUX,MUX0); // ^

	set(ADCSRA,ADIE); // set interrupt bit

	set(ADCSRA,ADEN); // demask ADC interrupt

	

}

// Enable and Begin Timer 1
void timer1_enable(void){
	
	OCR1A = ocr1a_constant;
	
	set(TCCR1B,CS12); // pre-scale clock /1024 to make timer speed ~2kHz
	clear(TCCR1B,CS11); // ^
	set(TCCR1B,CS10); // ^
	
	clear(TCCR1B,WGM13); // timer mode up to OCR1A
	set(TCCR1B,WGM12); // ^
	clear(TCCR1A,WGM11); // ^
	clear(TCCR1A,WGM10); // ^
	
	set(TIMSK1,OCIE1A); // interrupt when OCR1A is reached
}

// Set Initial state of system to non-firing, non-charging (off)
void output_init(void)
{
	
	
	clear(PORTD,0); // turn off green light
	clear(PORTD,3); // turn off red light
	
	clear(PORTB,4); // turn N channel 1 off (charging off)
	clear(PORTB,5); // turn N channel 2 off (firing off)
	
}

// Begin Measurement of IR Light and charging of capacitor
void ADC_init(void)
{
	sei(); // enable global interrupts

	set(ADCSRA,ADSC); // begin conversion
	
	set(PORTD,3); // turn on red light to indicate charging is occurring
	
	set(PORTB,4); // turn on N channel 1 pin (charging on)

	TCNT1 = 0x0000; // reset timer 1 count to zero
}


/************************************************************
Interrupts
************************************************************/

// ADC New Value Interrupt
ISR(ADC_vect)
{
	//OCR1A = ocr1a_constant/1023.0*ADC; // set the timer (OCR1A) based on the distance from the IR light
	set(ADCSRA,ADSC); // begin conversion again
}
	
// Timer1 OCR1A Interrupt
ISR(TIMER1_COMPA_vect)
{
	
	clear(PORTB,4); // turn off N channel 1 pin (charging off)
	
	set(PORTD,0); //turn on green light to indicate firing is ready
	clear(PORTD,3); // turn off red light to indicate charging is completed
	
	firing_ready = 1;
}

/************************************************************
End of Program
************************************************************/