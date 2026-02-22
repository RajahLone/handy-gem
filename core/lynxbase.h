#ifndef LYNXBASE_H
#define LYNXBASE_H

#include <cstdio>

//
// bank0        - Cartridge bank 0
// bank1        - Cartridge bank 1
// ram          - all ram
// cpu          - system memory as viewed by the cpu
//
enum EMMODE {bank0,bank1,ram,cpu};

class CLynxBase
{
  // Function members
  
public:
  virtual ~CLynxBase() {};
  
public:
  virtual void    Reset(void) {};
  virtual bool    ContextLoad(FILE *fp) { return 0; };
  virtual bool    ContextSave(FILE *fp) { return 0; };
  
  virtual void    Poke(ULONG addr,UBYTE data)=0;
  virtual UBYTE   Peek(ULONG addr)=0;
  virtual void    PokeW(ULONG addr, UWORD data) {}; // ONLY mSystem overloads these, they are never use by the clients
  virtual UWORD   PeekW(ULONG addr) {return 0;};
  virtual void    BankSelect(EMMODE newbank){};
  virtual ULONG   ObjectSize(void) {return 1;};
};

#endif
