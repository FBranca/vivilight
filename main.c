/**
 * @brief  Vivilight - a DIY dynamic lighting system
 *
 * @author F. Branca, a long time ago
*/
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <math.h>

/// Get a baudrate (relative to crystal frequency)
#define BAUDRATE(baud) (unsigned short) ((F_CPU / (16 * (long)baud)) - 1)

#define NB_CANAUX   3 //!< Number of channels (3 channel : 1 at the top of the screen and 2 on each side)
#define NB_COULEURS 3 //!< Number of colors (Red, Green, Blue)

/// Number of PWM signals needed
#define NB_PWM (NB_CANAUX * NB_COULEURS)

#define MOSFET_ON(port, bit)  *port |= bit   //!< Macro to drive an ouput high
#define MOSFET_OFF(port, bit) *port &= ~bit  //!< Macro to drive an ouput low

/// Pointer on a function of type void function(void)
typedef void (*Tpf) (void);

/// memorize a value for an output
typedef struct 
{
	volatile uint8_t *port;	//!< Pointer to the port
	uint8_t bit;            //!< Index of the bit used to drive the output
	uint8_t valeur;         //!< Current ratio of the PWM
} TComposante;

volatile uint8_t timeout;
volatile uint8_t offset;

/// Declaration of each color/channel
TComposante pwmRecu[NB_PWM] =
{
	{ &PORTB, _BV(2), 0x90}, // bleu,  canal 1
	{ &PORTB, _BV(1), 0x40}, // bleu,  canal 2
	{ &PORTD, _BV(3), 0xC0}, // bleu,  canal 3
	{ &PORTB, _BV(3), 0x40}, // Vert,  canal 1
	{ &PORTB, _BV(0), 0x90}, // Vert,  canal 2
	{ &PORTD, _BV(4), 0x40}, // Vert,  canal 3
	{ &PORTB, _BV(4), 0x90}, // Rouge, canal 1
	{ &PORTD, _BV(6), 0x40}, // Rouge, canal 2
	{ &PORTD, _BV(5), 0x90}  // Rouge, canal 3
};



/*
void DesactiveEtProgramme (void)
{
	while ((pcourante >= pwmApplique) && (pcourante->valeur <= TCNT0))
	{
		MOSFET_OFF (pcourante->port, pcourante->bit);
		pcourante--;
	}
	if (pcourante >= pwmApplique)
	{
		OCR0A = pcourante->valeur;
	}
}*/

/// Hardware timer 0
ISR(TIMER0_OVF_vect)
{
	if (timeout > 0)
	{
		timeout --;
		if (timeout==0)
		{
			offset = 0;
		}
	}
}


void tri (TComposante *ppwm)
{
    TComposante tmp;
	register uint8_t ok;
	register uint8_t i;
	
	do
	{
		ok = 1;
		for (i=NB_PWM-2; i>0; i--)
		{
			if (ppwm[i+1].valeur > ppwm[i].valeur)
			{
				tmp = ppwm[i+1];
				ppwm[i+1] = ppwm[i];
				ppwm[i] = tmp;
				ok = 0;
			}
		}
	} while (!ok);
}


/**
 * @brief Initialize the UART
*/
void USART_Init( register unsigned short baud )
{
	UBRRH = (unsigned char) (baud >> 8);
	UBRRL = (unsigned char) baud;
	UCSRB = _BV(RXEN);
	UCSRC = _BV(UCSZ1)|_BV(UCSZ0);
}

/**
 * @brief Initialize registers
*/
void init (void)
{
	DDRB = 0x1F;
	DDRD = 0x7F;
	
	TIMSK  = _BV(TOIE0);
	TCCR0B = _BV(CS01) | _BV(CS00);
	
	USART_Init (BAUDRATE(9600));
}


/**
 * @brief Test function ...
 *
 * I don't remember what it does. It's not in my habits to keep dead code but I make an exception here.
 * It might contain genius content that will revolutionize our way of programming (or it might not ...).
*/
void test2 (void)
{
	uint8_t i,k,v,d,s;
	
	k = 0;
	v = 0;
	s = 1;
	d=100;
	while (1)
	{
		for (i=0; i<NB_PWM; i++)
		{
			if (pwmRecu[i].valeur <= TCNT0)
				MOSFET_OFF  (pwmRecu[i].port, pwmRecu[i].bit);
			else
				MOSFET_ON (pwmRecu[i].port, pwmRecu[i].bit);
		}

		d--;
		if (d==0)
		{
			d=20;
			if (s)
			{
				v++;
				if (v>200)
					s = 0;
			}
			else
			{
				v --;
				if (v==255)
				{
					v=0;
					s = 1;
					k++;
					if (k>=NB_PWM)
					{
						k = 0;
					}
				}
			}
			pwmRecu[k].valeur = v;
		}
	}
}


/**
 * @brief Main loop
 *
 * Receive data from serial port and apply them to each PWM output.
*/
void loop (void)
{
	uint8_t i;
	
	// Infinite loop
	while (1)
	{
	    // There are not enough PWM output on the uC.
		// We handle this by hand
		for (i=0; i<NB_PWM; i++)
		{
			if (pwmRecu[i].valeur <= TCNT0)
				MOSFET_OFF  (pwmRecu[i].port, pwmRecu[i].bit);
			else
				MOSFET_ON (pwmRecu[i].port, pwmRecu[i].bit);
		}

		// If a character is available on the serial line.
		if (UCSRA & _BV(RXC))
		{
			timeout = 200;
			pwmRecu[offset++].valeur = UDR;
			if (offset>=NB_PWM)
			{
				offset=0;
			}
		}
	}
}


int main (void)
{
	CLKPR = _BV(CLKPCE);	
	CLKPR = 0;

	init();
	sei();
	loop();
	test2();
	
	return (0);
}
