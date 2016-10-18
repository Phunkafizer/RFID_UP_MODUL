 /*
	rfid.cpp
	rfid class for decoding EM4102 & FDX-B transponder
	S. Seegel, post@seegel-systeme.de
	feb 2011
	last update okt 2016
	http://opensource.org/licenses/gpl-license.php GNU Public License
*/

#include "rfid.h"
#include "global.h"
#include <avr/interrupt.h>
#include "tagmanager.h"
#include <string.h>
#include <util/crc16.h>

#define F_RFID 125000UL	//RFID carrier frequency
#define CLOCKSPERBIT_EM4100 64
#define CLOCKSPERBIT_FDXB 32

#if F_CPU * 16UL / PRESCAL / F_RFID < 10
	#error RFID: Timer too slow
#endif

#if F_CPU * 64UL / PRESCAL / F_RFID > 180
	#error RFID: Timer too fast
#endif

/* some infos
	64 clocks per bit:
	BITRATE = F_RFID / 64 =~ 1.953 kbits/s
	short pulse = 1 / BITRATE / 2 =~ 256 µS
	long pulse = 1 / BITRATE =~ 512 µS
	
	32 clocks per bit (e.g. FDX-B)
	BITRATE = F_RFID / 32 =~ 3.906 kbits/s
	short pulse = 1 / BITRATE / 2 =~ 128 µS
	long pulse = 1 / BITRATE =~ 256 µS
*/

#define MANCHESTER_MIDDLE(x) (uint8_t) (3 * x * F_CPU / F_RFID / PRESCAL / 4)

volatile uint8_t Rfid::buffer[16];
bool Rfid::em_flag;
bool Rfid::fdx_flag;

Rfid::Rfid() {
	currentTag = TAG_NONE;
}

void Rfid::Init(void) {
	#if defined(SHDDDR)
	SHDDDR |= 1<<SHDPINX; //EM4095 shutdown pin
	#endif
	
	MCUCR |= 1<<ISC00; //Any logical change on INT0 generates an interrupt request
	GICR |= 1<<INT0; //External interrupt request 0 enable (demod_out from RFID chip)
}

void Rfid::Execute(void) {
	static uint8_t state = 0;
	
	if (em_flag)
	{
		if (decodeEm4100()) {
			timer.SetTime(200);
			if (state == 0) {
				state = 1;
				currentTag = TAG_EM4100;
			}
		}
		em_flag = false;
	}
	
	if (fdx_flag) {
		if (decodeFDX()) {
			timer.SetTime(200);
			if (state == 0) {
				state = 1;
				currentTag = TAG_FDXB;
			}
		}
		fdx_flag = false;
	}
	
	GICR |= 1<<INT0; //External interrupt request 0 enable (demod_out from RFID chip)
	
	if (timer.IsFlagged())
		state = 0;
}

enum tag_t Rfid::getTag(uint8_t *tag)
{
	switch (currentTag) {
		case TAG_EM4100:
			memcpy(tag, this->tagdata, 5);
			tag[5] = 0x00;
			break;
		
		case TAG_FDXB:
			memcpy(tag, this->tagdata, 6);
			break;
			
		default:
			break;
	}
	
	enum tag_t res = currentTag;
	currentTag = TAG_NONE;
	return res;
}

void Rfid::irq() {
	rfidcnt_t cnt = RFIDCNTREG;
	static rfidcnt_t old_cnt;
	uint8_t diff = (rfidcnt_t) (cnt - old_cnt);
	old_cnt = cnt;
	static uint8_t bitstate;
	static uint8_t bitcount;
	static uint8_t thresh;
	static uint8_t mode = 1; //1 = em4100, 2 = fdx_b
	static uint16_t fifo;
	static bool insync = false;
	
	if (diff > MANCHESTER_MIDDLE(CLOCKSPERBIT_EM4100))
		if (mode != 1) {
			thresh = MANCHESTER_MIDDLE(CLOCKSPERBIT_EM4100);
			mode = 1;
			insync = 0;
			bitcount = 0;
		}
		
	if (diff < MANCHESTER_MIDDLE(CLOCKSPERBIT_FDXB))
		if (mode != 2) {
			thresh = MANCHESTER_MIDDLE(CLOCKSPERBIT_FDXB);
			mode = 2;
			insync = 0;
			bitcount = 0;
		}
	
	if ((uint8_t)(diff) > thresh) 
		bitstate = 0;
	else {
		if (bitstate == 0)
			bitstate = 1;
		else
			bitstate ^= 0x03;
	}
	
	if (bitstate == 1)
		return;
		
	bitcount++;
	
	switch (mode) {
	case 1: //EM4100
		fifo <<= 1;
		//manchester decoding
		if (bit_is_clear(PIND, PD2)) {
			fifo |= 1;
		}
		
		if (insync) {
			if ((bitcount % 8) == 0)
				buffer[bitcount / 8 - 1] = fifo;
			
			if (bitcount == 64) {
				insync = 0;
				GICR &= ~(1<<INT0);
				em_flag = true;
			}
		}
		else {
			if ((fifo & 0x3FF) == 0x1FF) {
				insync = true;
				bitcount = 9;
				buffer[0] = 0xFF;
			}
		}
		break;
	
	case 2: //FDX-B
		fifo >>= 1;
		//biphase marc
		if (!bitstate)
			fifo |= 0x8000;
			
		if (insync) {
			if ((bitcount % 8) == 0) {
				buffer[bitcount / 8 - 1] = fifo >> 8;
			}
			
			if (bitcount == 128) {
				insync = 0;
				GICR &= ~(1<<INT0);
				fdx_flag = true;
			}
		}
		else
			if ((fifo & 0xFFE0) == 0x8000) {
				insync = true;
				bitcount = 11;
				buffer[0] = 0x00;
			}
		break;
	}
}

bool Rfid::decodeEm4100() {
	uint8_t ibit, obit, i, lp, p;
	
	//calc line parities
	i=0;
	p=0;
	lp=0;
	for (ibit=9; ibit<59; ibit++)
	{
		lp ^= (buffer[ibit / 8] >> (7 - (ibit % 8))) & 0x01;
		
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
		lp ^= (buffer[ibit / 8] >> (7 - (ibit % 8))) & 0x01;
		p |= lp;
	}
	
	//all parity bits ok ?
	if (!p)
	{
		tagdata[0] = 0; tagdata[1] = 0; tagdata[2] = 0; tagdata[3] = 0; tagdata[4] = 0;
		//Remove parity bits
		ibit = 9;
		for (obit=0; obit<40; obit++)
		{
			if ((buffer[ibit / 8] >> (7 - (ibit % 8))) & 0x01)
			tagdata[obit / 8] |= 1 << (7 - (obit % 8));
			
			ibit++;
			if (((obit + 1) % 4) == 0)
			ibit++;
		}
		return true;
	}
	
	return false;
}

bool Rfid::decodeFDX()
{
	/*
	Example raw data:
	LSB first!
	Hex: 00 AC FD BB A1 56 8B E1 01 02 9E BF 1A 20 40 80 
	     <-- 0 -> <-- 1 -> <-- 2 -> <-- 3 -> <-- 4 -> <-- 5 -> <-- 6 -> <-- 7 -> <-- 8 -> <-- 9 -> <- 10 -> <- 11 -> <- 12 -> <- 13 -> <- 14 -> <- 15 ->
	Bin: 00000000 10101100 11111101 10111011 10100001 01010110 10001011 11100001 00000001 00000010 10011110 10111111 00011010 00100000 01000000 10000000
	     HHHHHHHH AAAAAHHH BBBB1AAA CCC1BBBB DD1CCCCC E1DDDDDD 1FFEEEEE GGFFFFFF 000000H1 00000010 JJJJJ1I0 KKKK1JJJ LLL1KKKK MM1LLLLL N1MMMMMM 1NNNNNNN

         1 = stuffing bits
		 0 = always 0 bits
		 H = Header,
		 A..E = 38 bits national code
		 F..G = 10 bits country code
		 H = data block status flag
		 I = animal application indicator flag
		 J..K = 16 bits CCITT CRC (kermit) over previous 64 bit payload
		 L..N = 24 bits extra data if bit H is set
		 
	Decoded national code:
	Hex: 16     5A       0D       BF       B5   
	Bin: 010110 01011010 00001101 10111111 10110101
		 EEEEEE DDDDDDDD CCCCCCCC BBBBBBBB AAAAAAAA
	Dec: 096000130997
		 
	Decoded country code:
	Hex: 03  84
	Bin: 11 10000100
	     GG FFFFFFFF
	Dec: 900
	*/
	
	//collect national code
	uint64_t wd = (buffer[1] & 0xF8) >> 3 | (buffer[2] & 0x07) << 5;
	wd |= ((uint64_t) buffer[2] & 0xF0) << 4 | ((uint64_t) buffer[3] & 0x0F) << 12;
	wd |= ((uint64_t) buffer[3] & 0xE0) << 11 | ((uint64_t) buffer[4] & 0x1F) << 19;
	wd |= ((uint64_t) buffer[4] & 0xC0) << 18 | ((uint64_t) buffer[5] & 0x3F) << 26;
	wd |= ((uint64_t) buffer[5] & 0x80) << 25 | ((uint64_t) buffer[6] & 0x1F) << 33;
	
	//collect 10 bit countrycode
	wd |= ((uint64_t) buffer[6] & 0x60) << 33 | ((uint64_t) buffer[7]) << 40;
	
	//collect flags
	wd |= ((uint64_t) buffer[8] & 0x02) << 47 | ((uint64_t) buffer[10] & 0x02) << 62;
	
	//collect CRC
	uint16_t crc = ((uint16_t) buffer[10] & 0xF8) >> 3 | ((uint16_t) buffer[11] & 0x07) << 5;
	crc |= ((uint16_t) buffer[11] & 0xF0) << 4 | ((uint16_t) buffer[12] & 0x0F) << 12;
	
	uint16_t calccrc = 0;
	for (int i=0; i<8; i++)
		calccrc = _crc_ccitt_update(calccrc, (wd >> (i * 8)) & 0xFF);
		
	if (calccrc == crc) {
		for (uint8_t i=0; i<6; i++)
			tagdata[i] = (wd >> (i * 8)) & 0xFF;
		
		return true;
	}

	return false;
}

