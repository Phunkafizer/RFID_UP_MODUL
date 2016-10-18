/*-----------------------------------------------------------------------------
FILENAME: tagmanager.h
AUTHOR: S. Seegel, post@seegel-systeme.de
DATE: 22.7.2008
VERSION: 1.0
LICENSE: GPL v3
DESCRIPTION: Declaration of Tagmanager class, which manages the tags in
eeprom, i.e. checking and adding.
-----------------------------------------------------------------------------*/

#ifndef tagmanager_h
#define tagmanager_h

#include "task.h"
#include "timer.h"
#include "rfid.h"

typedef struct {
	enum tag_t tagtype;
	uint8_t data[6];
} tag_item_t;

enum tagmanagermode_e {TMM_IDLE, TMM_LOOKUP};

class Tagmanager: public Task
{
	private:
		enum tagmanagermode_e mode;
		tag_item_t tag;
		uint16_t eepromIdx;
		tag_item_t *tag_ptr;
		Timer timer;
		uint16_t numtags;
		uint16_t maxnumtags;
		bool admintag;
		void EnterAdminTagMode(void);
		void GiveAccess(void);
		void DenyAccess(void);
	protected:
	public:
		Tagmanager(void);
		void Init(void);
		void Execute(void);
		void ProcessTag(tag_item_t *tag);
		uint16_t AddTag(tag_item_t *tag);
		bool ReadTag(uint16_t idx, tag_item_t *tag);
		uint16_t WriteTag(tag_item_t *tag, uint16_t idx);
};

extern Tagmanager tagmanager;

#endif
