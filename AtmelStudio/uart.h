#ifndef uart_h
#define uart_h

#include "task.h"
#include "timer.h"
#include "bus.h"

class Uart: public Task
{
	private:
		void SendReplyFrame(void);
		void CmdAddTag(void);
		void CmdReadTag(void);
		void CmdWriteTag(void);
		bool rxdataflag;
	protected:
		void Execute(void);
		
	public:
		Uart(void);
		void SendData(uint8_t *data, uint8_t len);
		bool MediaBusy(void);
		uint8_t GetRxData(uint8_t **data);
};

extern Uart uart;
#endif
