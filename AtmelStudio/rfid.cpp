/*
	rfid.cpp
	rfid class for decoding EM4102 & FDX-B transponder
	S. Seegel, post@seegel-systeme.de
	feb 2011
	last update apr 2016
	http://opensource.org/licenses/gpl-license.php GNU Public License
*/

#include "rfid.h"
#include "global.h"
#include <avr/interrupt.h>
#include "tagmanager.h"

Rfid rfid;

#define CNTREG TCNT2
typedef uint8_t cnt_t;

#define F_RFID 125000UL	//RFID carrier frequency
#define TIMERPRESCALER 64 //just for checking if timerresolution in correct range

#if F_CPU * 16UL / TIMERPRESCALER / F_RFID < 10
	#error RFID: Timer too slow
#endif

#if F_CPU * 64UL / TIMERPRESCALER / F_RFID > 180
	#error RFID: Timer too fast
#endif

/* some infos
	EM4102 BITRATE = F_RFID / 64 =~ 1.953 kbits/s
	EM4102 short pulse = 1 / BITRATE / 2 =~ 256 µS
	EM4102 long pulse = 1 / BITRATE =~ 512 µS
	
	FDX-B BITRATE = F_RFID / 32 =~ 3.906 kbits/s
	FDX-B short pulse = 1 / BITRATE / 2 =~ 128 µS
*/

#define TIMER_PRESCALER 32 //Needed for manchester decoding

#define MANCHESTER_LONG (F_CPU / F_RFID * 64 / TIMER_PRESCALER)
#define MANCHESTER_SHORT (F_CPU / F_RFID * 64 / TIMER_PRESCALER / 2)
#define MANCHESTER_MIDDLE ((uint8_t) ((MANCHESTER_LONG + MANCHESTER_SHORT) / 2))
#define MANCHESTER_MIN ((uint8_t) (MANCHESTER_SHORT - 5))
#define MANCHESTER_MAX ((uint8_t) (MANCHESTER_LONG + 5))

void Rfid::Init(void)
{
	#if defined(SHDDDR)
	SHDDDR |= 1<<SHDPINX; //EM4095 shutdown pin
	#endif
	
	TCCR0 = 1<<CS01 | 1<<CS00; //Timer/Counter0 prescaler 64
	MCUCR |= 1<<ISC00; //Any logical change on INT0 generates an interrupt request
	GICR |= 1<<INT0; //External interrupt request 0 enable (demod_out from RFID chip)
}

void Rfid::Execute(void)
{
	uint8_t ibit, obit, i, lp, p;
	
	if (rawtag_flag)
	{
		//calc line parities
		i=0;
		p=0;
		lp=0;
		for (ibit=9; ibit<59; ibit++)
		{
			lp ^= (rawtag[ibit / 8] >> (7 - (ibit % 8))) & 0x01;
			
			i++;
			if (i==5)
			{
				i=0;
				p |= lp;
			}
		}
			
		//calc line parities
		for (i=0; i<4; i++) //Check 4 columns
		{
			lp = 0;
			for (ibit=9+i; ibit<9+55+i; ibit+=5)
				lp ^= (rawtag[ibit / 8] >> (7 - (ibit % 8))) & 0x01;
			p |= lp;
		}
		
		//all parity bits ok ?
		if (!p)
		{
			tag[0] = 0; tag[1] = 0; tag[2] = 0; tag[3] = 0; tag[4] = 0;
			//Remove parity bits
			ibit = 9;
			for (obit=0; obit<40; obit++)
			{
				if ((rawtag[ibit / 8] >> (7 - (ibit % 8))) & 0x01)
					tag[obit / 8] |= 1 << (7 - (obit % 8));
				
				ibit++;
				if (((obit + 1) % 4) == 0)
					ibit++;
			}
			
			if (tag_flag == 0)
				tag_flag = 1;

			timer.SetTime(200);
		}
		
		rawtag_flag = 0;
		GICR |= 1<<INT0;
	}
	
	if (timer.IsFlagged())
	{
		tag_flag = 0;
	}
	
	if (tag_flag == 1)
	{
		tag_flag = 2;
		tagmanager.ProcessTag(tag);
	}
}

void Rfid::InjectByte(uint8_t b)
{
	if (bytecnt < 8)
	{
		rawtag[bytecnt++] = b;
		
		if (bytecnt == 8)
		{
			GICR &= ~(1<<INT0);
			rawtag_flag = 1;
		}
	}
}

void Rfid::TagStart(void) {
	bytecnt = 0;
}

ISR(INT0_vect)
{
	cnt_t cnt = CNTREG;
	static uint16_t timebase;
	static uint8_t bitstate = 0;
	static cnt_t old_cnt;
	uint8_t diff = (cnt_t) (cnt - old_cnt);
	static uint16_t fifo;
	static bool insync = false;
	static uint8_t bitcount;
	

	if (diff > timebase) //this is a long pulse after a short pulse
		bitstate = 0;
	else
		if (diff < (timebase / 2)) //this is a short pulse after a long pulse
			bitstate = 1;
		else //same pulse length (within 0.75x and < 1.5x of last length)
			bitstate ^= 0x03;
	
	timebase = 3 * diff / 2;	
	old_cnt = cnt;
	
	if (bitstate == 1)
		return;
		
	
	/*if ((uint8_t)(cnt - old_cnt) < MANCHESTER_MIDDLE)
		return;
	old_cnt = cnt;*/
		



	fifo <<= 1;
	bitcount++;
		
	//manchester
	if (bit_is_clear(PIND, PD2))
		fifo |= 1;
	
	/*
	//biphase marc
	if (!bitstate)
		fifo |= 1;*/
		
	if (insync) {
		if ((bitcount % 8) == 0)
			UDR = fifo;
		
		if (bitcount == 64) {
			insync = 0;
			volatile int32_t d;
			for (d=0; d<200000; d++);
		}
	}
	else
	{
		if ((fifo & 0x3FF) == 0x1FF) {
			UDR = fifo >> 3;
			insync = true;
			bitcount = 11;
		}
	}
		
	return;
	
	/*static cnt_t old_cnt;
	static uint16_t sync;
	static uint8_t bitcount;
	static uint8_t insync = 0;
	
	cnt_t diff = (uint8_t)(TCNT0 - old_cnt);
	
	
	
	//TEMP
	old_cnt = TCNT0;
	UDR = diff;
	return;
	
	if ((diff > MANCHESTER_MAX) || (diff < MANCHESTER_MIN))
	{
		insync = 0;
		bitcount = 0;
		return;
	}
	
	if (diff > MANCHESTER_MIDDLE)
	{
		old_cnt = TCNT0;
	
		sync <<= 1;
		bitcount++;
		
		if (bit_is_clear(PIND, PD2))
			sync |= 1;
		
		if (insync)
		{
			if ((bitcount % 8) == 0)
				rfid.InjectByte(sync);
			
			if (bitcount == 64)
				insync = 0;
		}
		else
			if ((sync & 0x3FF) == 0x1FF)
			{
				insync = 1;
				bitcount = 9;
				rfid.TagStart();
				rfid.InjectByte(sync);
			}
	}*/
}

