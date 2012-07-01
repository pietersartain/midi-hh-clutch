/*=============================================================================
midi-hh-clutch.c
Pieter E. Sartain  pesartain@googlemail.com
pesartain.com
=============================================================================*/

/******************************************************************************
Project level include files.
******************************************************************************/

#include <ecog.h>
#include <registers.h>
#include <stdio.h>
#include <core_lib.h>

/******************************************************************************
Private constants and types.
******************************************************************************/

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
Main function / entry point
******************************************************************************/

int main(int argc, char * argv[])
{    
  
  long kickthreshold = 0xFFFFFE4A;

	while (1)
	{
		adc_read(0);
				
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

/***************************************************************
ADC Code Copyright (c) 2006 Cyan Technology Ltd
***************************************************************/

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
      // clear ADC conversion complete flag
   		fd.adc.ctrl.int_clr = 1;

		if (adc_skip < NUM_DISCARD)
		{
      // Discard first samples after changing mux setup
			adc_skip++;
		}
		else
		{
      // Wraparound - signal is out of range
			if (status & 0x2000) adc_errs++;

      // Count reading
			adc_count++;
   		value = (int)(status & 0x0fff);  // store ADC value (clear rdy and ovfl bits)
			value = (value << 4) >> 4;       // Sign extend
			adc_sum += value;                // Add to sum

      if (adc_count >= NUM_SAMPLES)  // If enough samples have been collected..
      {
        complete = 1;                // ..set complete flag..
        fd.adc.ctrl.int_dis = 1;     // ..disable ADC interrupts
        if (adc_errs >= NUM_ERROR) adc_error = E_WRAP;
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
