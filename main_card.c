/* This program is for a 16F84 Gold Card that may be charged with tokens.
   Every time the card is inserted into its reader, some tokens are withdrawn.
   The card may be charged from a third party with 2, 4 or 8 tokens.           																		   
   main() and parseInt() written by Pethrus Gärdborn and Ramcin Oudishu 	  
   All other code originates from sample file smartkey.c which is
   located at 
   https://www.kth.se/social/course/IE1206/page/smartcard-utrustningen/        */

/*
SmartCard contact
             __ __ __
       +5V  |C1|   C5| Gnd
            |__|   __|
      MCLR  |C2|  |C6| 
            |__|  |__|
       OSC  |C3|  |C7| RB7/PGD I/O -><- Txd/Rxd half duplex 
            |__|  |__|
            |C4|  |C8|
            |__|__|__|

*/

/*
  SERIAL COMMUNICATION
  ============================
  One start bit, one stop bit, 8 data bit, no parity = 10 bit.
  Baudrate: 9600 baud => 104.167 usec. per bit.
  serial output PORTB.7  half duplex!
  serial input  PORTB.7  half duplex!
*/

#include "16F84.h"
#define MAX_STRING 16  /* string input max 15 characthers */
#define WITHDRAW 2	   /* fixed withdraw amount */
#pragma config |= 0x3ff1

/* Function prototypes                                   */
void initserial( void );
void putchar( char );
char getchar( void );
void string_out( const char * string ); 
void string_in( char * );
bit check_candidate( char * input_string, const char * candidate_string ); 
void delay( char );
void putchar_eedata( char data, char EEPROMadress );
char getchar_eedata( char EEPROMadress ); 
void puts_eedata( char * );
void gets_eedata( char * );

char parseInt( char * );

void main( void)
{
	char i, c, d, sum;
	char input_string[MAX_STRING]; /* 15 char buffer for input string */   
	bit compare;
	bit compareMoney;
	delay(50);  /* delay to stabilize power */

	initserial();

	for (i = 0;;) {
		string_in( &input_string[0] );	/* Receive string signal.
						"has money?" from card reader to check if tokens on card. 
						Or "2", "4" or "8" to put new balance on card. */
		
		/* Compare input and question text. Respond by charging balance or withdrawal. */
		compare = check_candidate( &input_string[0], "has money?" );
		compareMoney = check_candidate( &input_string[0], "2" ); 
		compareMoney = compareMoney | check_candidate( &input_string[0], "4" );
		compareMoney = compareMoney | check_candidate( &input_string[0], "8" );
		
		delay(150);     /* give the lock time to get ready */
		
		/*  If a request is made for a transaction */
		if(compare == 1) {
			/* Withdraw "tokens" only once during a session. 
			For that reason we use i. */
			if (!i) {
				i++;
				d = getchar_eedata(0); // Read number of tokens from memory
				
				if (d >= WITHDRAW) { // If enough tokens, signal card reader.
					string_out("yes\r\n");
					// Withdraw and store the new value in memory
					d -= WITHDRAW;	
					putchar_eedata(d, 0);
				} else { // If not enough tokens, signal card reader.
					string_out("no\r\n");
				}
			}
		} 
		
		/* If a request to charge the card is made*/
		else if (compareMoney == 1) {
			sum = parseInt( &input_string[0] ); // Convert string to number
			putchar_eedata(sum, 0); // Store the requested new balance on card.
		}
	}
}

/********************
     FUNCTIONS
     =========
*********************/
/* Parses a string as an unsigned decimal integer. */
char parseInt( char * input_string ) {
	signed char i = 0;
	char sum = 0;
	char power = 1;
	char n;
	
	// Determine length of string +1
	while (input_string[i] != '\0')	
		i++;
	
	// Iterate through the string starting from the end, excluding NULL
	while ( --i >= 0 ) {
		n = input_string[i] - 48;
		n *= power;
		sum += n;
		power *= 10;
	}
	
	return sum;
}

void initserial( void ) /* initialise serialcom port */
{
   PORTB.7 = 1;
   TRISB.7 = 1;  /* input mode */
}


void putchar( char d_out )  /* sends one char */
{
   char bitCount, ti;
   TRISB.7 = 0; /* output mode */
   PORTB.7 = 0; /* set startbit */
   for ( bitCount = 10; bitCount > 0 ; bitCount-- )
        {
         /* 104 usec at 3,58 MHz (5+27*3-1+9=104) */  
         // ti = 27; do ; while( --ti > 0);
         /* 104 usec at 4 MHz (5+30*3-1+1+9=104)  */         
          ti = 30; do ; while( --ti > 0); nop();   
          Carry = 1;           /* stopbit                        */
          d_out = rr( d_out ); /* Rotate Right through Carry     */
          PORTB.7 = Carry;
        }
        nop2(); nop2();
   return; /* all done */
}


char getchar( void )  /* recieves one char */
{
   /* One start bit, one stop bit, 8 data bit, no parity = 10 bit. */
   /* Baudrate: 9600 baud => 104.167 usec. per bit.                */
   TRISB.7 = 1; /* set input mode */
   char d_in, bitCount, ti;
   while( PORTB.7 == 1 )  /* wait for startbit */ ;
   /* delay 1,5 bit is 156 usec                         */ 
   /* 156 usec is 156 op @ 4 MHz ( 5+47*3-1+2+9=156)    */
   ti = 47; do ; while( --ti > 0); nop2();  
   for( bitCount = 8; bitCount > 0 ; bitCount--)
       {
        Carry = PORTB.7;
        d_in = rr( d_in);  /* rotate carry */
        /* delay 1 bit is 104 usec                       */ 
        /* 104 usec is 104 op @ 4 MHz (5+30*3-1+1+9=104) */  
        ti = 30; do ; while( --ti > 0); nop();  
        }
   return d_in;
}

void string_in( char * input_string )
{
   char charCount, c;
   for( charCount = 0; ; charCount++ )
       {
         c = getchar( );     /* input 1 character             */
         input_string[charCount] = c;  /* store the character */
         //putchar( c );     /* don't echo the character      */
         if( (charCount == (MAX_STRING-1))||(c=='\r' )) /* end of input   */
           {
             input_string[charCount] = '\0';  /* add "end of string"      */
             return;
           }
       }
}


void string_out(const char * string)
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


bit check_candidate( char * input_string, const char * candidate_string )
{
   /* compares input buffer with the candidate string */
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
  using the TMR0 timer 
*/
{
    OPTION = 2;  /* prescaler divide by 8        */
    do  {
        TMR0 = 0;
        while ( TMR0 < 125)   /* 125 * 8 = 1000  */
            ;
    } while ( -- millisec > 0);
}

void puts_eedata( char * s )
{
/* Put s[] -string in EEPROM-data */
char i, c;
for(i = 0;  ; i++) 
    {
      c = s[i];
      /* Write EEPROM-data sequence                          */
      EEADR = i;          /* EEPROM-data adress 0x00 => 0x40 */ 
      EEDATA = c;         /* data to be written              */
      WREN = 1;           /* write enable                    */
      EECON2 = 0x55;      /* first Byte in comandsequence    */
      EECON2 = 0xAA;      /* second Byte in comandsequence   */
      WR = 1;             /* write begin                     */
      while( EEIF == 0) ; /* wait for done ( EEIF=1 )        */
      WR = 0;             /* write done                      */
      WREN = 0;           /* write disable - safety first    */
      EEIF = 0;           /* Reset EEIF bit in software      */
      /* End of write EEPROM-data sequence                   */
      if(c == '\0') break;  
    }
}


void gets_eedata( char * s )
{
/* Get EEPROM-data and put it in s[] */
char i;
for(i = 0; ;i++)
    {
      /* Start of read EEPROM-data sequence                */
      EEADR = i;       /* EEPROM-data adress 0x00 => 0x40  */ 
      RD = 1;          /* Read                             */
      s[i] = EEDATA;   /* data to be read                  */
      RD = 0;
      /* End of read EEPROM-data sequence                  */  
      if( s[i] == '\0') break;
    }
}


char getchar_eedata( char EEPROMadress )
{
  /* Get char from specific EEPROM-adress */
      /* Start of read EEPROM-data sequence                */
      char temp;
      EEADR = EEPROMadress;  /* EEPROM-data adress 0x00 => 0x40  */ 
      RD = 1;                /* Read                             */
      temp = EEDATA;
      RD = 0;
      return temp;           /* data to be read                  */
      /* End of read EEPROM-data sequence                        */  
}


void putchar_eedata( char data, char EEPROMadress )
{
  /* Put char in specific EEPROM-adress */
      /* Write EEPROM-data sequence                          */
      EEADR = EEPROMadress; /* EEPROM-data adress 0x00 => 0x40 */ 
      EEDATA = data;        /* data to be written              */
      WREN = 1;             /* write enable                    */
      EECON2 = 0x55;        /* first Byte in comandsequence    */
      EECON2 = 0xAA;        /* second Byte in comandsequence   */
      WR = 1;               /* write                           */
      while( EEIF == 0) ;   /* wait for done (EEIF=1)          */
      WR = 0;
      WREN = 0;             /* write disable - safety first    */
      EEIF = 0;             /* Reset EEIF bit in software      */
      /* End of write EEPROM-data sequence                     */
} 

