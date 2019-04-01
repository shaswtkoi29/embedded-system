#include "msp.h"
#include <stdio.h>


int right_count = 0;
int left_count = 0;
int right_dir;
int left_dir;
int p_3_5_state;
int p_5_0_state;
int p_5_1_state;
int p_5_2_state;
int p_3_5_state_prev = 0;
int p_5_0_state_prev = 0;
int p_5_1_state_prev = 0;
int p_5_2_state_prev = 0;

int count_initial = 0;
int state = 0;
int count_bool = 1;

void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer

	//Setting the wheel directions (P1.6 and P1.7 determine directions)
	P1SEL0 = 0; //Set all of PORT 1 to be GPIO
	P1SEL1 = 0;
	P1DIR |= (BIT6 | BIT7); //Set P1.6 and P1.7 to be GPIO outputs
	P1OUT &= ~BIT7; //Set both motors to go forward
	P1OUT |= BIT6;

	//Setting up the Port 2 PWM
	P2DIR  |=  0b11000000;         // Makes P2.6 and P2.7 Outputs
	P2SEL0 |=  0b11000000;         // Make P2.6 and P2.7 Timer0A functions (choose PWM as the source)
	P2SEL1 &= ~0b11000000;         // Make P2.6 and P2.7 Timer0A functions
	TIMER_A0->CCTL[0] = 0x0090;    // CCI0 toggle
	TIMER_A0->CCR[0] =  20000;    // Period of the PWM. Set to 20000 in this program
	TIMER_A0->EX0 =     0x0000;    // Divide by 1
	TIMER_A0->CCTL[3] = 0x0040;    //
	TIMER_A0->CCR[3] =    4720;    //Duty for 2.6 (Right Wheel). This will determine wheel speed
	TIMER_A0->CCTL[4] = 0x0040;    //
	TIMER_A0->CCR[4] =    5000;    //Duty for 2.7 (Left wheel). This will determine wheel speed
	TIMER_A0->CTL =     0x0230;    // SMCLK=12MHz, divide by 1, up-down mode

	//Set P5.0, P5.1, P5.2, and P3.5 to be GPIO inputs
	P5SEL0 &= ~(BIT0 | BIT1 | BIT2);
	P5SEL1 &= ~(BIT0 | BIT1 | BIT2);
	P3SEL0 &= ~BIT5;
	P3SEL1 &= ~BIT5;
	P5DIR &= ~(BIT0 | BIT1 | BIT2);
	P3DIR &= ~BIT5;

	//Calibration on 12/5: For going for feet, set PWM RIGHT to 4650/20000 and PWM LEFT to 5000/20000 and wait for 3000 counts

	while(1)
	{
	    p_3_5_state = (P3IN & BIT5);
	    p_5_0_state = (P5IN & BIT0);
	    p_5_1_state = (P5IN & BIT1);
	    p_5_2_state = (P5IN & BIT2);

	    if(p_3_5_state != p_3_5_state_prev)
	    {
	        left_count++;
	    }

	    if(p_5_0_state != p_5_0_state_prev)
	    {
	        right_count++;
	    }

	    p_3_5_state_prev = p_3_5_state;
	    p_5_0_state_prev = p_5_0_state;
	    p_5_1_state_prev = p_5_1_state;
	    p_5_2_state_prev = p_5_2_state;

	    //First four feet
	    if(count_bool)
	    {
	        count_initial = left_count;
	        count_bool = 0;
	    }

	    if(left_count < count_initial + 3000 && state == 0)
	    {
	        P1OUT &= ~BIT7; //Set both motors to go forward
	        P1OUT &= ~BIT6;
	        if(left_count == count_initial + 2999)
	        {
	            state = 1;
	            count_bool = 1;
	        }
	    }

	    //90 degree turn
	    //count_initial = left_count;
	    if(left_count < count_initial + 380 && state == 1)
	    {
	        P1OUT &= ~BIT7;
	        P1OUT |= BIT6;
	        if(left_count == count_initial + 379)
	        {
	            state = 2;
	            count_bool = 1;
	        }
	    }

	    //Second four feet
	    //count_initial = left_count;
	    if(left_count < count_initial + 3000 && state == 2)
	    {
	        P1OUT &= ~BIT7; //Set both motors to go forward
	        P1OUT &= ~BIT6;
	        if(left_count == count_initial + 2999)
	        {
	            state = 0;
	            count_bool = 1;
	        }
	    }

	}



}


