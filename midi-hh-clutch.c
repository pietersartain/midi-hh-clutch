/***************************************************************

Date: September 8th, 2006.
Copyright (c) 2006 Cyan Technology Limited. All Rights Reserved.

Cyan Technology Limited, hereby grants a non-exclusive license
under Cyan Technology Limited's copyright to copy, modify and
distribute this software for any purpose and without fee,
provided that the above copyright notice and the following
paragraphs appear on all copies.

Cyan Technology Limited makes no representation that this
source code is correct or is an accurate representation of any
standard.

In no event shall Cyan Technology Limited have any liability
for any direct, indirect, or speculative damages, (including,
without limiting the forgoing, consequential, incidental and
special damages) including, but not limited to infringement,
loss of use, business interruptions, and loss of profits,
respective of whether Cyan Technology Limited has advance
notice of the possibility of any such damages.

Cyan Technology Limited specifically disclaims any warranties 
including, but not limited to, the implied warranties of
merchantability, fitness for a particular purpose, and non-
infringement. The software provided hereunder is on an "as is"
basis and Cyan Technology Limited has no obligation to provide
maintenance, support, updates, enhancements, or modifications.

****************************************************************/

/*=============================================================================
Cyan Technology Limited

FILE - gpio.c
    Example eCOG1 application.

DESCRIPTION
    Test GPIO pins and interrupts.

MODIFICATION DETAILS
=============================================================================*/



/******************************************************************************
Project level include files.
******************************************************************************/


#include <ecog.h>
#include <registers.h>
#include <stdio.h>
#include <core_lib.h>

//#include <driver_lib.h>

/******************************************************************************
Include files for public interfaces from other modules.
******************************************************************************/



/******************************************************************************
Declaration of public functions.
******************************************************************************/



/******************************************************************************
Private constants and types.
******************************************************************************/

#define LED0 0
#define LED1 1
#define LED2 2
#define LED3 3

// GPIO irq config
#define OFF 0
#define LOW 2
#define HIGH 3
#define FALLING 4
#define RISING 5
#define BOTH 7

#define SELECTED FALLING

// Error codes
enum
{
	E_NONE = 0,
	E_WRAP,
	E_OVFL
};

#define SAMPLE_RATE	8000              // Sample frequency in Hz (fixed for on-chip adc)

#define NUM_SAMPLES	1                // Number of samples to average
#define NUM_DISCARD	4                // Number of samples to discard after setting mux

#if (NUM_SAMPLES >= 4)
#define NUM_ERROR	(NUM_SAMPLES / 4)
#else
#define NUM_ERROR	1
#endif

/******************************************************************************
Declaration of static functions.
******************************************************************************/

void adc_start(unsigned int config);
void adc_stop(void);
void adc_read(unsigned int config);


void __irq_code pattern(void);
void __irq_code gpio_wr(unsigned int gpio, unsigned int out);



/******************************************************************************
Global variables.
******************************************************************************/

int adc_skip = 0;
int adc_count = 0;
long adc_sum = 0;
long adc_prev = 0;
int adc_error = E_NONE;
int adc_errs = 0;
char complete = FALSE;      // This indicates when adc data has been stored


int kick_down = 0;

/******************************************************************************
Module global variables.
******************************************************************************/



/******************************************************************************
Definition of API functions.
******************************************************************************/


/******************************************************************************
NAME
    main

SYNOPSIS
    int main(int argc, char * argv[])

FUNCTION
    Sit in a loop and call function pattern when the interrupt signals.

RETURNS
    Exit code.
******************************************************************************/

int main(int argc, char * argv[])
{    
    long kickthreshold = 0xFFFFFE4A;
	char ach;
	char bch;
	
    // Set gpio13 as input (disable output)
    fd.io.gp12_15_out.dis13 = 1;
    // Enable gpio interrupt on sw1
    fd.io.gp12_15_cfg.int13 = SELECTED;
    
    // Disable port C for test
    fd.port.dis.c = 1;
    
	// Send a mid C at mid volume
	// To channel 10 (0-index)
	// 9A 45 45
	/**
	duart_a_write(0x99);
	duart_a_write(0x45);
	duart_a_write(0x45);
	/**/
	
	// Send a program change message to ch 10
	/**
	duart_a_write(0xC9);
	duart_a_write(0x60); // Kit 97
	/**/
	//sleep(1);

	while (1)
	{
		adc_read(0);
		//duart_a_write(0x99);
		
		//ach = (int)((adc_sum & 0xFF00) >> 8);
		//bch = (int)(adc_sum & 0x00FF);
		
		//duart_a_write(ach);
		//duart_a_write(bch);
		//duart_a_write(kickthreshold);
		
		
		if ( (adc_sum < kickthreshold) && (kick_down == 0) )
		{
			duart_a_write(0x99);
			duart_a_write(0x24); // note 36
			duart_a_write(0x45);
			kick_down = 1;
		}
		
		if (adc_sum > kickthreshold)
		{
			kick_down = 0;
		}
		

	}
    
    return (0);
}


/******************************************************************************
NAME
    int_handler

SYNOPSIS
    void __irq_entry int_handler(void)

FUNCTION
    Interrupt handler for gpio.

RETURNS
    Nothing.
******************************************************************************/

void __irq_entry int_handler(void)
{
    unsigned int i = 0;
    volatile unsigned int temp;
    
    // Clear gpio interrupt
    temp = rg.io.gp8_15_sts;
    temp = ~temp;
    
    // Disable gpio interrupt
    //fd.io.gp8_15_cfg.int13 = OFF;
    
    pattern();
    for (i=0; i<10000; i++)
        temp = i;
}


/******************************************************************************
NAME
    pattern

SYNOPSIS
    static void pattern(void)

FUNCTION
    Put the next pattern on the LEDs and the LCD.

RETURNS
    Nothing.
******************************************************************************/

void __irq_code pattern(void)
{
    static unsigned int count;

    count = (count+1) % 16;

    gpio_wr(LED3, (count & 8) ? 0 : 1);
    gpio_wr(LED2, (count & 4) ? 0 : 1);
    gpio_wr(LED1, (count & 2) ? 0 : 1);
    gpio_wr(LED0, (count & 1) ? 0 : 1);
}


// Copy of gpio_wr() called from irq service routine
void __irq_code gpio_wr(unsigned int gpio, unsigned int out)
{
    if (gpio < 29)
    {
        *(&rg.io.gp0_3_out + (gpio >> 2)) = ((out) ? 1 : 2) << ((gpio & 3) << 2);
    }
}

// ADC end-of-conversion ISR
// The ADC sample is stored in memory, and an index variable is incremented.
// If enough samples have been taken, then a flag is set,
// and ADC interrupts are disabled until the next set is requested.
//
void __irq_entry adc_handler (void)
{
	unsigned int status = rg.adc.sts;
	int value;
	
	if ((status & 0x8000) != 0)
	{
   		fd.adc.ctrl.int_clr = 1;        	// clear ADC conversion complete flag

		if (adc_skip < NUM_DISCARD)
		{
			adc_skip++;						// Discard first samples after changing mux setup
		}
		else
		{
			if (status & 0x2000)			// Wraparound - signal is out of range
			{
				adc_errs++;
			}
			adc_count++;				    // Count reading
   			value = (int)(status & 0x0fff); // store ADC value (clear rdy and ovfl bits)
			value = (value << 4) >> 4;	    // Sign extend
			adc_sum += value;			    // Add to sum

   			if (adc_count >= NUM_SAMPLES)   // If enough samples have been collected..
   			{
      			complete = 1; 				// ..set complete flag..
   				fd.adc.ctrl.int_dis = 1;    // ..disable ADC interrupts
				
				/*
				if ( (adc_prev > 0x50) && (adc_sum <= 0x50) )
				{
					kick_down = 1;
				}
				
				if ( (adc_prev <= 0x50) && (adc_sum > 0x50) )
				{
					kick_down = 0;
				}
				*/
				
				

				if (adc_errs >= NUM_ERROR)
					adc_error = E_WRAP;
			}
   		}
	}
}

void adc_start(unsigned int config)
{
	adc_skip = 0;
	adc_count = 0;
	adc_prev = adc_sum;
	adc_sum = 0;
	adc_error = E_NONE;
	adc_errs = 0;
	complete = 0;
	
	rg.adc.cfg = config;					// Select adc input channel

	fd.adc.ctrl.int_clr = 1;				// Clear any adc interrupt
	fd.adc.ctrl.int_en = 1;					// Enable adc interrupt
}


void adc_stop(void)
{
	fd.adc.ctrl.int_clr = 1;				// Clear any adc interrupt
	fd.adc.ctrl.int_dis = 1;				// Disable adc interrupt
}


void adc_read(unsigned int config)
{
	adc_start(config);		// start ADC sampling
    while (0 == complete);
}
