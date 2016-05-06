/*-----------------------------------------------------------------------------
FILENAME: bus.h
AUTHOR: S. Seegel, post@seegel-systeme.de
DATE: 17.8.2008
VERSION: 1.0
LICENSE: GPL v3
DESCRIPTION: Declaration of Bus class (RS485 communication)
-----------------------------------------------------------------------------*/

#ifndef bus_h
#define bus_h

#include "task.h"
#include "timer.h"

#define CMD_OPEN_RELAY 0x10
#define CMD_TAG_REQUEST 0x20
#define CMD_READ_TAG 0x30
#define CMD_READ_TAG_REP 0x31
#define CMD_WRITE_TAG 0x40
#define CMD_IO_EVENT 0x50

class Bus: public Task
{
	private:
		uint16_t hostid;
		uint8_t lasttxframeid;
		uint8_t lastrxframeid;
		uint8_t last_txlen;
		uint8_t tx_left;
		Timer sifstimer;
		uint8_t l1_txbuf[30];
		bool lastL1ack;
		bool txpending; //set when tx frame is in buf which was not already acknowledged
		volatile uint8_t l1_txbuflen;
		void InitSifsTimer(void);
		void BuildL1Frame(uint8_t *data, uint8_t len, bool ack);
		uint16_t CalcCrc(uint8_t data, uint8_t len);
		void AddL1Data(uint8_t *data, uint8_t len, uint16_t *crc);
		void StartL1TX(void);
	protected:
		void Execute(void);
	public:
		Bus(void);
		void SendTagRequest(uint8_t *tag, bool accessed);
		void SendEmptyAck(void);
		void SendIOEvent(uint8_t event);
};

extern Bus bus;

#endif
