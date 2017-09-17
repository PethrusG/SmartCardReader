/* smartlock.c  question and answer, compare strings and unlock */
/* B Knudsen Cc5x C-compiler - not ANSI-C                       */

#include "16F690.h"
#pragma config |= 0x00D4
#define MAX_STRING 16

/* Function prototypes */
void initserial( void );
void  putchar( char );
char getchar( void );     /* Special! nonblocking when card removed  */
void OverrunRecover(void);
void string_in( char * ); /* no extra echo, local echo by connection */
void string_out( const char * string );
bit check_password( char * input_string, const char * candidate_string );
void delay( char );

void main( void)
{
   
   TRISC.3 = 1;  /* RC3 card contact is input       */
   ANSEL.7 = 0;  /* RC3 digital input               */
   TRISC.2 = 0;  /* RC2 lock (lightdiode) is output */
   PORTC.2 = 0;  /* RC2 initially locked            */

   while(1)
     {
       while( PORTC.3 == 0) ; /* wait for card insertion */
       delay(100); /* card debounce */
       delay(50);  /* extra delay   */      

       while( PORTC.3 == 1) {
	      PORTC.2 = 1;
		  delay(150);
		  PORTC.2 = 0;
		  delay(150);
	   } /* wait for card removal */

       delay(10);

       //PORTC.2 = 0;  /* lock again now when card is out  */
       delay(100);   /* card debounce */
    }
}




/********************
     FUNCTIONS
     =========
*********************/

void initserial( void )  /* initialise PIC16F690 serialcom port */
{
   /* One start bit, one stop bit, 8 data bit, no parity. 9600 Baud. */

   TXEN = 1;      /* transmit enable                   */
   SYNC = 0;      /* asynchronous operation            */
   TX9  = 0;      /* 8 bit transmission                */
   SPEN = 1;

   BRGH  = 0;     /* settings for 6800 Baud            */
   BRG16 = 1;     /* @ 4 MHz-clock frequency           */
   SPBRG = 25;

   CREN = 1;      /* Continuous receive                */
   RX9  = 0;      /* 8 bit reception                   */
   ANSELH.3 = 0;  /* RB5 digital input for serial_in   */
}


void OverrunRecover(void)
{  
   char trash;
   trash = RCREG;
   trash = RCREG;
   CREN = 0;
   CREN = 1;
}


void putchar( char d_out )  /* sends one char */
{
   while (!TXIF) ;   /* wait until previus character transmitted   */
   TXREG = d_out;
   return; /* done */
}

char getchar( void )  /* recieves one char */
{
   char d_in = '\r';
   while ( !RCIF && PORTC.3 ) ;  /* wait for character or card removal */
   if(!RCIF) return d_in;
   d_in = RCREG;
   return d_in;
}

void string_in( char * string ) 
{
   char charCount, c;
   for( charCount = 0; ; charCount++ )
       {
         c = getchar( );        /* input 1 character         */
         string[charCount] = c; /* store the character       */
         // putchar( c );       /* don't echo the character  */
         if( (charCount == (MAX_STRING-1))||(c=='\r' )) /* end of input */
           {
             string[charCount] = '\0'; /* add "end of string" */
             return;
           }
       }
}


void string_out(const char * string )
{
  char i, k;
  for(i = 0 ; ; i++)
   {
     k = string[i];
     if( k == '\0') return;   /* found end of string */
     putchar(k); 
   }
  return;
}

bit check_password( char * input_string, const char * candidate_string )
{
   /* compares input with the candidate string */
   char i, c, d;
   for(i=0; ; i++)
     {
       c = input_string[i];
       d = candidate_string[i];
       if(d != c ) return 0;       /* no match    */
         if( d == '\0' ) return 1; /* exact match */
     }
}


void delay( char millisec)
/* 
  Delays a multiple of 1 milliseconds at 4 MHz
  using the TMR0 timer by B. Knudsen
*/
{
    OPTION = 2;  /* prescaler divide by 8        */
    do  {
        TMR0 = 0;
        while ( TMR0 < 125)   /* 125 * 8 = 1000  */
            ;
    } while ( -- millisec > 0);
}



/********************
      HARDWARE
      ========
*********************/

/*
           ___________  ___________ 
          |           \/           |
    +5V---|Vdd      16F690      Vss|---GND
          |RA5        RA0/AN0/(PGD)|-x ->- (PK2Rx)
          |RA4            RA1/(PGC)|-x -<- (PK2Tx)
    SW1---|RA3/!MCLR/(Vpp)  RA2/INT|
          |RC5/CCP              RC0|
          |RC4                  RC1|
  CrdIn->-|RC3                  RC2|->-LED (lock)
          |RC6                  RB4|
          |RC7               RB5/Rx|-<-SerIn
 SerOut-<-|RB7/Tx               RB6|
          |________________________|

Card Oscillator OSC 4 MHz ELFA 74-560-07

Card contact
                   __ __ __
              +5V |C1|   C5| GND
                  |__|   __|
        !MCLR/PGM |C2|  |C6| 
                  |__|  |__|
          OSC/PGC |C3|  |C7| IO/PGD 
                  |__|  |__|
                  |C4|  |C8|
                  |__|__|__|
                Contact CrdIn
				
All communications are tied together:

                  +5
                   |    --K<A-- = diodes
                  10k
                   |
           CrdIO---+--1k---(PK2Rx)
                   +--A>K--(PK2Tx)
      SerOut--K<A--+-------SerIn
*/

