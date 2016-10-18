/*-----------------------------------------------------------------------------
FILENAME: bus.cpp
AUTHOR: S. Seegel, post@seegel-systeme.de
DATE: 17.8.2008
VERSION: 1.0
LICENSE: GPL v3
DESCRIPTION: Implementation of Bus class (RS485 communication)
-----------------------------------------------------------------------------*/

#include "bus.h"
#include "global.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include <stdlib.h>
#include <util/crc16.h>
#include "relay.h"
#include "tagmanager.h"
#include "button.h"

#define SLOT_TIME 10
#define SIFS 20
#define DIFS 150

#define BAUD 9600UL

#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUD)
#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))
#error Baudrate error > 1%!
#endif

Bus bus;

static volatile uint8_t rxbuf[30];
static volatile uint8_t rxlen;
static volatile bool rxflag;
static volatile bool rxbusy;
static volatile uint8_t txbuf[30];
static volatile uint8_t txbufpointer;
static volatile uint8_t txlen;
static volatile bool txbusy = false; //true while sending
static Timer rxbusytimer;

typedef struct
{
	uint16_t destadr;
	uint16_t srcadr;
	uint8_t frameid;
	bool ack;
} frameheader_t;

Bus::Bus(void)
{
	UCSRB = 1<<RXCIE | 1<<TXCIE | 1<<TXEN | 1<<RXEN;
	UCSRC = 1<<URSEL | 1<<UCSZ1 | 1<<UCSZ0;
	UBRRH = UBRR_VAL >> 8;
	UBRRL = UBRR_VAL & 0xFF;
	rxbusy = false;
	hostid = eeprom_read_word(&eeconfig.hostadr);
	lasttxframeid = 0x00;
	lastrxframeid = 0x00;
	srand(hostid);
	tx_left = 0;
	l1_txbuf[l1_txbuflen++] = ESC;
	l1_txbuf[l1_txbuflen++] = STX;
	txpending = false;
}

void Bus::InitSifsTimer(void)
{
	if ((!MediaBusy()) && (tx_left > 0))
		sifstimer.SetTime(DIFS);//sifstimer.SetTime(((rand() % 10)) * SLOT_TIME + DIFS);
	else
		sifstimer.SetTime(0);
}

void Bus::Execute(void)
{
	static bool old_busy = false;
	bool busy = MediaBusy();
	if (old_busy != busy)
	{
		old_busy = busy;
			
		if (!lastL1ack)
			InitSifsTimer();
		else
			sifstimer.SetTime(SIFS);
	}
	
	if (tx_left > 0)
	{
		if (sifstimer.IsFlagged())
		{
			SendData(l1_txbuf, l1_txbuflen);
			tx_left--;
		}
	}
	
	if (rxbusytimer.IsFlagged())
		rxbusy = false;
	
	if (rxflag)
	{
		rxflag = false;
		
		if (rxlen < 3)
			return;
		
		//CheckCRC
		uint16_t crc = 0xFFFF;
		uint8_t i;
		for (i=0; i<rxlen; i++)
			crc = _crc_ccitt_update(crc, rxbuf[i]);
		
		if (crc != 0)
			return;

		rxlen -= 2; //substract crc
		
		uint8_t* rxdata;
		frameheader_t *rxhead;
		rxhead = (frameheader_t *) rxbuf;
		
		if (rxhead->destadr == hostid) {
			if (rxhead->ack)
			{
				tx_left = 0;
				txpending = false;
			}
			else
			tx_left = 1; //Force to send 1 ack frame
		
			if (rxhead->frameid != lastrxframeid)
			{
				lastrxframeid = rxhead->frameid;
				if (rxlen - sizeof(frameheader_t) == 0)
				return;
			
				rxdata = (uint8_t*) rxhead + sizeof(frameheader_t);
				tag_item_t tag;

				switch (*rxdata++) {
					case CMD_SET_ADDRESS:
						if (button.IsProgMode())
							eeprom_write_word(&eeconfig.hostadr, rxdata[0] | (uint16_t) rxdata[1] << 8);
						BuildL1Frame(0, 0, true); //Send back ACK frame
						break;
				
					case CMD_SET_PULSE_LEN:
						eeprom_write_byte(&eeconfig.triggertime, rxdata[0]);
						BuildL1Frame(0, 0, true); //Send back ACK frame
						break;
				
					case CMD_OPEN_RELAY:
						relay.TriggerTime(rxdata[0]);
						BuildL1Frame(0, 0, true); //Send back ACK frame
						break;
				
					case CMD_READ_TAG: {
						uint8_t rep[1 + 2 + sizeof(tag_item_t)];
						rep[0] = CMD_READ_TAG_REP;
						rep[1] = rxdata[0];
						rep[2] = rxdata[1];
						tagmanager.ReadTag(rxdata[0] | rxdata[1] << 8, &tag);
						memcpy(&rep[3], &tag, sizeof(tag_item_t));
						BuildL1Frame(rep, sizeof(rep), true);
						break;
					}
				
					case CMD_WRITE_TAG: {
						uint8_t rep[2];
						memcpy(&tag, &rxdata[2], sizeof(tag_item_t));
						uint16_t idx = tagmanager.WriteTag(&tag, rxdata[0] | rxdata[1] << 8);
						rep[0] = idx & 0xFF;
						rep[1] = idx << 8;
						BuildL1Frame(rep, sizeof(rep), true);
						break;
					}
				
					case CMD_ADD_TAG:
						memcpy(&tag, &rxdata[0], sizeof(tag_item_t));
						tagmanager.AddTag(&tag);
						BuildL1Frame(0, 0, true);
						break;
				}
			}
			else
			{
				//Frame already received, just send last txframe again
				if (tx_left > 0)
				StartL1TX();
			}
		}
	}
}

void Bus::AddL1Data(uint8_t *data, uint8_t len, uint16_t *crc)
{
	while (len--)
	{
		if (*data == ESC)
			l1_txbuf[l1_txbuflen++] = *data;
		l1_txbuf[l1_txbuflen++] = *data;
		if (crc != 0)
			*crc = _crc_ccitt_update(*crc, *data);
		data++;
	}
}

void Bus::BuildL1Frame(uint8_t *data, uint8_t len, bool ack) {
	if ((tx_left > 0) && (!ack))
		return;
	
	uint16_t crc = 0xFFFF;
	
	frameheader_t head;
	
	head.destadr = 0x00;
	head.srcadr = hostid;
	if (ack)
		head.frameid = lasttxframeid;
	else {
		lasttxframeid++;
		head.frameid = lasttxframeid;	
	}

	head.ack = ack;
	
	l1_txbuflen = 2;
	AddL1Data((uint8_t*) &head, sizeof(head), &crc);
	AddL1Data(data, len, &crc);
	
	AddL1Data((uint8_t *) &crc, 2, 0);
	l1_txbuf[l1_txbuflen++] = ESC;
	l1_txbuf[l1_txbuflen++] = ETX;
	
	lastL1ack = ack;
	StartL1TX();
}

void Bus::StartL1TX(void)
{
	if (!lastL1ack)
	{
		tx_left = 5;
		InitSifsTimer();
	}
	else
	{
		tx_left = 1;
		sifstimer.SetTime(SIFS);
	}
}

void Bus::SendData(uint8_t *data, uint8_t len)
{
	memcpy((uint8_t*) txbuf, data, len);
	txbufpointer = 0;
	txlen = len;
	RSDIRPORT |= 1<<RSDIRPINX;
	UCSRB |= 1<<UDRIE;
}

bool Bus::MediaBusy()
{
	return (rxbusy) || (bit_is_set(UCSRB, UDRIE));
}

void Bus::SendTagRequest(tag_item_t *tag, bool accessed)
{
	uint8_t data[1 + sizeof(tag_item_t) + 1]; //CMD, struct, accessed
	data[0] = CMD_TAG_REQUEST;
	memcpy(&data[1], tag, sizeof(tag_item_t));
	data[1 + sizeof(tag_item_t)] = accessed;
	BuildL1Frame(data, 1 + sizeof(tag_item_t) + 1, false);
}

void Bus::SendIOEvent(uint8_t event)
{
	return;
	//event: 0 = HW V3 Pinheader shortend
	
	uint8_t data[2];
	data[0] = CMD_IO_EVENT;
	data[1] = event;
	BuildL1Frame(data, sizeof(data), false);
}

ISR (USART_TXC_vect) {
	RSDIRPORT &= ~(1<<RSDIRPINX);
}

ISR (USART_UDRE_vect) {
	UDR = txbuf[txbufpointer++];
	if (txbufpointer == txlen)
		UCSRB &= ~(1<<UDRIE);
}

ISR (USART_RXC_vect) {
	register char c;
	static bool esc_mode = false;
	c = UDR;

	rxbusytimer.SetTime(5);
	rxbusy = true;
	
	if (esc_mode)
	{
		esc_mode = false;
		switch (c)
		{
			case STX:
				rxlen = 0;
				break;
			case ETX:
				rxflag = true;
				break;
			case ESC_ESC:
				rxbuf[rxlen++] = 27;
				break;
		}
	}
	else
		if (c == 27)
			esc_mode = true;
		else
			rxbuf[rxlen++] = c;
	
	if (rxlen >= sizeof(rxbuf))
		rxlen = 0;
}