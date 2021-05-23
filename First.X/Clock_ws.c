/* Clock_ws.c
 Version: 1.1
 Author : Sibtain Ali
 Date: 20/04/21
 Description: A clock with an implementation of a weather station
 
 
 PIC16F877A
 
 */






//PRIMARY CONFIGURATION

#pragma config FOSC = XT    // Set crystal oscillator        
#pragma config WDTE = OFF      // Watchdog timer OFF    
#pragma config PWRTE = OFF     // Power up timer OFF  
#pragma config BOREN = OFF     // Brown?out reset OFF  
#pragma config LVP = OFF       // Low voltage programming OFF
#pragma config CPD = OFF       // Code protection bit OFF  
#pragma config WRT = OFF       // Flash Program Memory Write Enable OFF
#pragma config CP = OFF    // Flash Program Memory Code Protection OFF
#include <xc.h>         // Required by the XC8 compiler    
#include "LCDdrive.h"
#include <string.h>
#include<stdbool.h>            // Import the boolean library
#define _XTAL_FREQ 3276800


//Function Prototypes
void setup(void);
void ADC_initialise(void);
signed int ADC_read(void);
void ADC_channel_select(unsigned char channel);
void updateClock (int hours,int minutes,int seconds,int clock_mode, bool evening);
void delay (unsigned  delay);
void convert_time(void);
void set_time(void);
void set_temp(void);
void tick_tock(void);

//Defining and initializing global variables
char hour = 13;           // set startup hour
char minute = 0;          // set startup minute
char second = 0;          // set startup second
bool PM = true;          // is this time set for evening?
char clock_mode = 1;      // set clock mode; 1 = 24-hr format, 2 = 12-hour format;

double temp_c = 24;     // Dummy values to the temp variables, these will be reset at start up;

/*
 * This methods configures the ports for the weather station
 */
void ADC_initialise(void){
 TRISA=0x02;    // RA0 and RA1 as input
 ADCON0 = 0x48;
 ADCON1 = 0x80;
 ADCON0bits.ADON =1;

}

/*This returns a range of values, "result" depending on the analog input
 */

signed int ADC_read(void){

    signed int result;
    __delay_us(20);
    ADCON0bits.GO =1;
    while (ADCON0bits.GO ==1);
    result = (ADRESH<<8)+ ADRESL;
    return(result);
    
}

/* This sets the ADC channel
 * A separate method has been used to introduce modularity to the code
 */
void ADC_channel_select(unsigned char channel){
    ADCON0bits.CHS = channel;
}

/*This method Updates the LCD with the given values of time
 *clock_mode: This parameter updates the clock accordingly; to the 24 or 12 hour format.
 * evening: This is a boolean switch which checks for evening or morning, so the clock updates accordingly 
 */

 void updateClock (int hours,int minutes,int seconds,int clock_mode, bool evening){
  
    PM= evening;       //set the PM value based on the given evening boolean
     
    LCD_cursor (0,0);
    
    if(hours<10) LCD_putch('0');
    LCD_display_value(hours);
    LCD_putch(':');
    if(minutes<10) LCD_putch('0');
    LCD_display_value(minutes);
    LCD_putch(':');
    if(seconds<10) LCD_putch('0');
    LCD_display_value(seconds);
    LCD_cursor (9,0);
    
    if(clock_mode ==2){
    if(PM){
    LCD_puts("PM");
    }else{
    LCD_puts("AM");
    }
    }
 }
\
 
 /*This method does the configuration of ports for the clock to use
  * Timer 1 is also configured here
  * LCD and ADC are both initialized
  * The clock starts with the given values
  */
 void setup(void){
     // Configure GPIO ports
   
    TRISD=0x07;    // Make PORTD  input
    TRISB=0x00;    // Make PORTB output
    PORTB=0x00;    // Clear all PORTS
    PORTD=0x00;
    
    T0IE = 1;
    OPTION_REG = 0X07;      //set timer 0 pre scaler to 256;
    INTCON = 0XA0;
    INTCONbits.GIE =1;
    
    LCD_initialise();
    ADC_initialise();   
    
    // Start the clock with these values;
    updateClock(hour,minute,second,clock_mode,PM);
    
     
}
 
 /*This method implements the timer 1 to introduce a delay of 1 second 
  */
void delay (unsigned  delay)
 {
    unsigned int x;
    
    T1CON =0x30;
    for (x = 0; x<2*delay; x++)
    {
     PIR1bits.TMR1IF = 0;
     T1CONbits.TMR1ON =0;
     TMR1H = 0x38;
     TMR1L = 0x00;
     T1CONbits.TMR1ON =1;
     while(!PIR1bits.TMR1IF);
     
    }
 }

/*This method includes code to use for conversion of time format
 */
void convert_time(void){
    if(clock_mode ==1){
        if(PM){
            hour = hour %12;
            LCD_puts("PM");        
        }
        else{
            LCD_puts("AM");  
        }
        clock_mode = 2;
    }
    else if(clock_mode == 2){
        if(PM){
            hour = hour +12;
            LCD_puts("  ");
        }
        else{
             LCD_puts("  ");
        }
        clock_mode = 1;
    }
   
  
}

/*This methods calculates and  displays the temperature on LCD
 *Includes code for conversion of Units of C to F.
 */
void set_temp(void){
 ADC_channel_select(1);
            temp_c = (ADC_read()*0.088) -40;        //Some maths to convert the ADC raw input to a range of desired temperatures
            double temp_f = ((temp_c *1.8)+32);
            if(RD0){
            LCD_clear();
            LCD_putsc("TEMP:  ");
            if(!RD1){
            LCD_display_value(temp_c);
            LCD_putch('C');
            }else{
            LCD_display_value(temp_f);
            LCD_putch('F');
            }
            }
}

/*Implements "convert_time()" to convert time based on input from buttons
 * This is skipped if temperature mode is used
 */
void set_time(void){
                                          
            if(RD2 ){
            convert_time();
        }
            if(!RD0){
            LCD_clear();
            updateClock(hour, minute, second, clock_mode, PM);  
            }
}

/*Increments the time values according to the clock mode
 * Updates time when a day is completed including automatic switching of AM & PM
 */
void tick_tock(void){
    //increment time and reset second and minute values
        second++;
        if(second > 59){ 
            second = 0;
            minute++;}
        if(minute > 59){
            minute = 0;
            hour++;
        }
        //change AM to PM when at the the end of a 12 hour range
        if(hour == 11 & minute == 59 & second == 59){
            PM=!PM;
        }
        
        //reset hour values for both 12 and 24 hour formats
        if(hour > 12){
            if(clock_mode ==1){
        if(hour > 23){
            hour =0;
        }
        }
            else{
                if(hour > 12){
                    hour = 1;

                }
            }
        }
    
}

void main(void)
{
      
    setup();
    while(1){ 
        delay(1);
       
        tick_tock();
        
        set_temp();
       
        set_time();
        
    }
    INTCONbits.T0IF = 0;
      
}
    
void __interrupt() alarmISR(void){
   

    if(TMR0IF){  
        while(temp_c <4){ 
            LCD_clear();
            temp_c = (ADC_read()*0.088) -40;
            TMR0IF = 0;   
            for( int x = 0; x <3; x ++){
              while(!TMR0IF);
              TMR0IF = 0; 
            }
            RB7 =!RB7;
        }
        RB7 = 0;
        TMR0IF = 0;
    } 
}














