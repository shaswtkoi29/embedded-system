#include "msp.h"
#include <stdio.h>

#define BIT17 (1<<17)

// Define the locations for some of the ADC registers.  This is a parallel method that works side by side
// with the "CMSIS" method of defines that involve pointers and
// structures, which I feel is unduely complicated.
// The following defines the hardware address's of a subset of the ADC setup registers.
// Reference:  MSP432P401R Data Sheet, table 6-31 "Precsion ADC Registers" (page 107 of the pdf file)
// Also see: section 22.3 "ADC Registers" of the MSP432P4xx Technical reference manual.
volatile long *adc14_control_register_0 =          (volatile long *)0x40012000 ;  // ADCTL0
volatile long *adc14_control_register_1 =          (volatile long *)0x40012004 ;  // ADCTL1
volatile long *adc14_memory_control_register_0 =   (volatile long *)0x40012018 ;  // ADCMEMCTL0
//volatile long *adc14_memory_control_register_1 =   (volatile long *)0x400 ;  // ADCMEMCTL1
//volatile long *adc14_memory_control_register_2 =   (volatile long *)0x ;  // ADCMEMCTL2
volatile long *adc14_memory_register_0 =           (volatile long *)0x40012098 ;  // ADC14MEM0
volatile long *adc14_interrupt_flag_0_register =   (volatile long *)0x40012144 ;  // ADC14IFGR0
volatile long *adc14_interrupt_enable_0_register = (volatile long *)0x4001213C ;  // ADC14IER0
volatile long *adc14_interrupt_enable_1_register = (volatile long *)0x4001213C ;  // ADC14IER1



// These "defines" allows us to write code using the exact same names that appear in the data sheet, like we can
// for the port setup registers.  Due to legacy and portability issues, the TI compiler sometimes uses
// different methods of defining address's in code.
#define ADC14CTL0     *adc14_control_register_0
#define ADC14CTL1     *adc14_control_register_1
#define ADC14MCTL0    *adc14_memory_control_register_0
//#define ADC14MCTL1    ADC14->MCTL[1];
#define ADC14MEM0     *adc14_memory_register_0
#define ADC14IFGR0    *adc14_interrupt_flag_0_register
#define ADC14IER0     *adc14_interrupt_enable_0_register
#define ADC14IER1     *adc14_interrupt_enable_1_register

int adc_value_middle; //For middle IR sensor
int adc_value_left;
int adc_value_right;

int count_right = 0;
int count_left = 0;


void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer



    // Configure P4.6 to be in ADC mode, not GPIO.  The analog input will come from "channel" 7, which is somehow
    // associated with port4, bit 6.  This selects the "secondary" function for this pin.
    P4SEL1 |= (BIT5 | BIT6 | BIT7);
    P4SEL0 |= (BIT5 | BIT6 | BIT7);

    // Set up the ADC.  These bits are all defined in the MSP432P4xx Technical Reference Manual...
    ADC14CTL0  = 0x00000000 ;    // Disable the ADC14 before changing any values.
    ADC14CTL0 |= BIT17;
    ADC14CTL0 &= 0xFFFFFFFD ;    // Note that in this bit mask, only bit 1 is a zero.
    ADC14CTL0 |= 0x00000010 ;    // ADC14 on
    ADC14CTL0 |= 0x04000000 ;    // Source signal from the sampling timer       ****

    // You figure out this one
    ADC14CTL1 =  0x00000030 ;    // ADC14MEM0, 8-bit, ref on, regular power
    ADC14MCTL0 = 0x00000007 ;    //  0 to 3.3V, channel A7
    ADC14->MCTL[1] = 0x00000008 ;    //Channel A8
    ADC14->MCTL[2] = 0x00000086 ;    //Channel A6

    ADC14IER0 =  0 ;             // no interrupts
    ADC14IER1 =  0 ;             // no interrupts
    ADC14CTL0 |= 0x00000002 ;    // enable    But doesn't the core have to be on? (bit 4).  Original code.

    //Setting the wheel directions (P1.6 and P1.7 determine directions)
    P1SEL0 = 0; //Set all of PORT 1 to be GPIO
    P1SEL1 = 0;
    P1DIR |= (BIT6 | BIT7); //Set P1.6 and P1.7 to be GPIO outputs
    P1OUT &= ~(BIT7); //Set both motors to go forward
    P1OUT &= ~BIT6;

    //Setting up the Port 2 PWM
    P2DIR  |=  0b11000000;         // Makes P2.6 and P2.7 Outputs
    P2SEL0 |=  0b11000000;         // Make P2.6 and P2.7 Timer0A functions (choose PWM as the source)
    P2SEL1 &= ~0b11000000;         // Make P2.6 and P2.7 Timer0A functions
    TIMER_A0->CCTL[0] = 0x0090;    // CCI0 toggle
    TIMER_A0->CCR[0] =  20000;    // Period of the PWM. Set to 20000 in this program
    TIMER_A0->EX0 =     0x0000;    // Divide by 1
    TIMER_A0->CCTL[3] = 0x0040;    //
    TIMER_A0->CCR[3] =    0000;    //Duty for 2.6 (Right Wheel). This will determine wheel speed
    TIMER_A0->CCTL[4] = 0x0040;    //
    TIMER_A0->CCR[4] =    000;    //Duty for 2.7 (Left wheel). This will determine wheel speed
    TIMER_A0->CTL =     0x0230;    // SMCLK=12MHz, divide by 1, up-down mode


    while(1)
    {

        //This is how you start ADC conversion.
        ADC14CTL0 |= 1;

        while(ADC14CTL0 & 0x00010000){}//wait until the ADC14 busy flag is down

        adc_value_middle = (ADC14MEM0 & 0x00003FFF); //get the 12-bit adc_value for the middle IR sensor
        while(ADC14CTL0 & 0x00010000){}//wait until the ADC14 busy flag is down

        adc_value_right = (ADC14->MEM[1] & 0x00003FFF);
        while(ADC14CTL0 & 0x00010000){}//wait until the ADC14 busy flag is down

        adc_value_left = (ADC14->MEM[2] & 0x00003FFF);


        if(adc_value_middle > 14000)
        {
            if(adc_value_right > adc_value_left)
            {
                left_turn();
                count_left++;
                
            }
            else if(adc_value_left > adc_value_right)
            {
                right_turn();
                count_right++;
            }
        }

        delay(10000);



    }

}

void delay(long n)
{
    while(n > 0) n--;
}

long abs(long n)
{
    if(n < 0) return -n;
    else return n;
}


void stationary_turn(int time, int dir) //For dir: 1 = Left and 0 = Right
{
    TIMER_A0->CCR[3] = 0; //Turn off both motors
    TIMER_A0->CCR[4] = 0;

    delay(100000); //Delay for wheels to stop

    if(dir == 1) //If a left turn
    {
        P1OUT |= BIT7; //Switch the direction of the left wheel
        TIMER_A0->CCR[3] = 3000; //Right wheel
        TIMER_A0->CCR[4] = 3000; //Left wheel
        delay(time*10000); //Delay for turn

        P1OUT &= ~BIT7; //Reset the wheel direction back to normal
    }

    if(dir == 0) //If a right turn
    {
        P1OUT |= BIT6; //Switch the direction of the right wheel
        TIMER_A0->CCR[3] = 3000; //Right wheel
        TIMER_A0->CCR[4] = 3000; //Left wheel

        delay(time*10000); //Delay for turn

        P1OUT &= ~BIT6; //Reset the wheel direction back to normal
    }

}

void right_turn()
{
    stationary_turn(85,0);
}

void left_turn()
{
    stationary_turn(88,1);
}

