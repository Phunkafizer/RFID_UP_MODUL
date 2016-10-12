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
#include "uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include <stdlib.h>
#include <util/crc16.h>
#include "relay.h"
#include "tagmanager.h"

#define SLOT_TIME 10
#define SIFS 20
#define DIFS 150

typedef struct 
{
	uint16_t destadr;
	uint16_t srcadr;
	uint8_t frameid;
	bool ack;
	
} frameheader_t;

Bus bus;

Bus::Bus(void)
{
	hostid = eeprom_read_word(&eeconfig.hostadr);
	lasttxframeid = lastrxframeid = 0x0000;
	srand(hostid);
	tx_left = 0;
	l1_txbuf[l1_txbuflen++] = ESC;
	l1_txbuf[l1_txbuflen++] = STX;
	txpending = false;
}

#include "led.h"

void Bus::InitSifsTimer(void)
{
	if ((!uart.MediaBusy()) && (tx_left > 0))
		sifstimer.SetTime(DIFS);//sifstimer.SetTime(((rand() % 10)) * SLOT_TIME + DIFS);
	else
		sifstimer.SetTime(0);
}

void Bus::Execute(void)
{
	static bool old_busy = false;
	if (old_busy != uart.MediaBusy())
	{
		old_busy = uart.MediaBusy();
		//if (!old_busy)
			//LEDOFF;
			
		if (!lastL1ack)
			InitSifsTimer();
		else
			sifstimer.SetTime(SIFS);
	}
	
	if (tx_left > 0)
	{
		if (sifstimer.IsFlagged())
		{
			//LEDRED;
			uart.SendData(l1_txbuf, l1_txbuflen);
			tx_left--;
		}
	}
	
	uint8_t* rxdata;
	frameheader_t *rxhead;
	uint8_t rxlen;
	
	if ((rxlen = uart.GetRxData((uint8_t**) &rxhead)))
	{
		if (rxhead->destadr != hostid)
			return;
			
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
			
			switch (*rxdata++)
			{
				case CMD_OPEN_RELAY:
					BuildL1Frame(0, 0, true); //Send back ACK frame 
					relay.TriggerTime(*((uint16_t*) rxdata));
					break;
					
				case CMD_READ_TAG:
					uint8_t data[8];
					data[0] = CMD_READ_TAG_REP;
					data[1] = rxdata[0];
					data[2] = rxdata[1];
					//tagmanager.ReadTag((rxdata[0]) | rxdata[1] << 8, &data[3]);
															
					BuildL1Frame(data, 8, true);
					break;
					
				case CMD_WRITE_TAG:
					//tagmanager.WriteTag(&rxdata[2], *((uint16_t*) rxdata));
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

void Bus::BuildL1Frame(uint8_t *data, uint8_t len, bool ack)
{
	if ((tx_left > 0) && (!ack))
		return;
	
	uint16_t crc = 0xFFFF;
	
	frameheader_t head;
	
	head.destadr = 0x00;
	head.srcadr = hostid;
	head.frameid = ++lasttxframeid;
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

void Bus::SendTagRequest(uint8_t *tag, bool accessed)
{
	uint8_t data[7];
	data[0] = CMD_TAG_REQUEST;
	memcpy(&data[1], tag, 5);
	data[6] = accessed;
	BuildL1Frame(data, 7, false);
}

void Bus::SendIOEvent(uint8_t event)
{
	//event: 0 = HW V3 Pinheader shortend
	
	uint8_t data[2];
	data[0] = CMD_IO_EVENT;
	data[1] = event;
	BuildL1Frame(data, sizeof(data), false);
}

