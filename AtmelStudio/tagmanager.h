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

#define MAX_NUM_TAGS ((64 * 128) / 5)

enum tagmanagermode_e {TMM_IDLE, TMM_LOOKUP};

class Tagmanager: public Task
{
	private:
		enum tagmanagermode_e mode;
		uint8_t tag[5];
		uint16_t eeprom_ptr;
		Timer timer;
		uint16_t numtags;
		uint16_t maxnumtags;
		bool admintag;
		void EnterAdminTagMode(void);
		void GiveAccess(void);
		void DenyAccess(void);
		void AddCurrentTag(void);
	protected:
	public:
		Tagmanager(void);
		void Init(void);
		void Execute(void);
		void ProcessTag(uint8_t *tag);
		uint16_t AddTag(uint8_t *tag);
		void ReadTag(uint16_t i, uint8_t *tag);
		uint8_t WriteTag(uint8_t *tag, uint16_t id);
};

extern Tagmanager tagmanager;

#endif
