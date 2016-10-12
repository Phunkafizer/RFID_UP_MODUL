#include "uart.h"
#include "global.h"
#include <string.h>
#include "tagmanager.h"
#include <avr/interrupt.h>
#include <util/crc16.h>

Uart uart;

#define BAUD 115200UL

#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUD)
#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))
#error Baudrate error > 1%!
#endif

static volatile uint8_t rxbuf[30];
static volatile uint8_t rxbufpointer;
static volatile bool rxflag;
static volatile bool rxbusy;

static volatile uint8_t txbuf[30];
static volatile uint8_t txbufpointer;
static volatile uint8_t txlen;

static volatile bool txbusy = false; //true while sending

static Timer rxbusytimer;

Uart::Uart(void)
{
	RSDIRDDR |= 1<<RSDIRPINX;
	UCSRB = 0<<RXCIE | 0<<TXCIE | 1<<TXEN | 0<<RXEN;
	UCSRC = 1<<URSEL | 1<<UCSZ1 | 1<<UCSZ0;
	UBRRH = UBRR_VAL >> 8;
	UBRRL = UBRR_VAL & 0xFF;
	rxbusy = false;
	rxdataflag = false;
}


#include "led.h"
void Uart::Execute(void)
{
	if (rxbusytimer.IsFlagged())
	{
		rxbusy = false;
	}
	
	uint16_t crc = 0xFFFF;
	uint8_t i;
	
	if (rxflag)
	{
		rxflag = false;
		
		if (rxbufpointer < 3)
			return;
		
		//CheckCRC
		for (i=0; i<rxbufpointer; i++)
			crc = _crc_ccitt_update(crc, rxbuf[i]);
			
		//if (crc != 0)
			//return;
			
		rxdataflag = true;
	
		rxbufpointer -= 2;
			
		/*if (*((uint16_t *) &rxbuf[0]) != hostid)
			return;
			
		if (*((uint16_t *) &rxbuf[2]) != lastframeid)
		{
			lastframeid = *((uint16_t *) &rxbuf[2]);
					
			switch (rxbuf[4])
			{
				case 0x01:
					CmdAddTag();
					break;
					
				case 0x03:
					CmdReadTag();
					break;
					
				case 0x04:
					CmdWriteTag();
					break;
					
				case 0x10:
					CmdReadBuffer();
					break;
			}
			
		}
		else
		{
			txlen = last_txlen;
			txbufpointer = 0;
			RSDIRPORT |= 1<<RSDIRPINX;
			UCSRB |= 1<<UDRIE;
		}*/
	}
}

uint8_t Uart::GetRxData(uint8_t **data)
{
	if (rxdataflag)
	{
		rxdataflag = false;
		*data = (uint8_t*) rxbuf;
		return rxbufpointer;
	}
	
	return 0;
}

void Uart::SendReplyFrame(void)
{
	/*txbuf[0] = 27;
	txbuf[1] = STX;
	txlen = 2;
	
	uint16_t crc = 0xFFFF;
	
	uint8_t i;
	for (i=0; i<rxbufpointer; i++)
	{
		crc = _crc_ccitt_update(crc, rxbuf[i]);
		
		if (rxbuf[i] == 0x1B)
			txbuf[txlen++] = 0x1B;
		txbuf[txlen++] = rxbuf[i];
	}
	
	if ((crc & 0xFF) == 0x1B)
		txbuf[txlen++] = 0x1B;
	txbuf[txlen++] = crc & 0xFF;
	if ((crc >> 8) == 0x1B)
		txbuf[txlen++] = 0x1B;
	txbuf[txlen++] = crc >> 8;
	
	txbuf[txlen++] = 27;
	txbuf[txlen++] = ETX;
	
	last_txlen = txlen;
		
	txbufpointer = 0;
		
	RSDIRPORT |= 1<<RSDIRPINX;
	UCSRB |= 1<<UDRIE;*/
}

void Uart::CmdAddTag(void)
{
	uint16_t result;
	
	if (rxbufpointer != 10)
		return;
		
	uint8_t *tag = (uint8_t *) &rxbuf[5];
		
	result = tagmanager.AddTag(tag);
		
	rxbuf[10] = result & 0xFF;
	rxbuf[11] = result >> 8;
	
	rxbufpointer += 2;
	
	SendReplyFrame();
}

void Uart::CmdReadTag(void)
{
	uint16_t i;
	
	if (rxbufpointer != 7)
		return;
	
	i = rxbuf[5] | (uint16_t) rxbuf[6] << 8;
	
	//tagmanager.ReadTag(i, (uint8_t *) &rxbuf[7]);
	
	rxbufpointer += 5;
	
	SendReplyFrame();
}

void Uart::CmdWriteTag(void)
{
	uint8_t result;
	
	if (rxbufpointer !=12)
		return;
		
	//result = tagmanager.WriteTag((uint8_t *) &rxbuf[5], rxbuf[10] | (uint16_t) rxbuf[11]);
	
	rxbuf[12] = result;
	rxbufpointer += 1;
	
	SendReplyFrame();
}

void Uart::SendData(uint8_t *data, uint8_t len)
{
	memcpy((uint8_t*) txbuf, data, len);
	txbufpointer = 0;
	txlen = len;
	RSDIRPORT |= 1<<RSDIRPINX;
	UCSRB |= 1<<UDRIE;
}

bool Uart::MediaBusy(void)
{
	return (rxbusy) || (bit_is_set(UCSRB, UDRIE));
}


ISR (USART_TXC_vect)
{
	RSDIRPORT &= ~(1<<RSDIRPINX);
}

ISR (USART_UDRE_vect)
{
	UDR = txbuf[txbufpointer++];
	if (txbufpointer == txlen)
		UCSRB &= ~(1<<UDRIE);
}

ISR (USART_RXC_vect)
{
	register char c;
	static bool esc_mode = false;
	c = UDR;

	rxbusytimer.SetTime(5);
	rxbusy = true;
	
	//LEDGREEN;
	
	
	if (esc_mode)
	{
		esc_mode = false;
		switch (c)
		{
			case STX:
				rxbufpointer = 0;
				break;
			case ETX:
				rxflag = true;
				break;
			case ESC_ESC:
				rxbuf[rxbufpointer++] = 27;
				break;
		}
	}
	else
		if (c == 27)
			esc_mode = true;
		else
			rxbuf[rxbufpointer++] = c;
			
	if (rxbufpointer >= sizeof(rxbuf))
		rxbufpointer = 0;
}
