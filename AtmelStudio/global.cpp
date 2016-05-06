/*-----------------------------------------------------------------------------
FILENAME: global.cpp
AUTHOR: S. Seegel, post@seegel-systeme.de
DATE: 22.7.2008
VERSION: 1.0
LICENSE: GPL v3
DESCRIPTION: Some C++ tools
-----------------------------------------------------------------------------*/

#include "global.h"

const uint8_t empty_tag[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

bool useexteeprom;

extern "C"
{
	#include <stdlib.h>
	
  void __cxa_pure_virtual() //needed for abstract classes
  {
  }
}

void *operator new(size_t size)
{
  return malloc(size);
}

void *operator new[](size_t size)
{
  return malloc(size);
}

void operator delete(void* ptr)
{
  free(ptr);
}

void operator delete[](void* ptr)
{
  free(ptr);
}
