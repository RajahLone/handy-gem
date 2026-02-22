#ifndef SYSBASE_H
#define SYSBASE_H

class CSystemBase
{
  // Function members
  
public:
  virtual ~CSystemBase() {};
  
public:
  virtual void    Reset(void)=0;
  virtual void    Poke_CPU(ULONG addr,UBYTE data)=0;
  virtual UBYTE   Peek_CPU(ULONG addr)=0;
  virtual void    PokeW_CPU(ULONG addr,UWORD data)=0;
  virtual UWORD   PeekW_CPU(ULONG addr)=0;
  
  virtual void    Poke_RAM(ULONG addr,UBYTE data)=0;
  virtual UBYTE   Peek_RAM(ULONG addr)=0;
  virtual void    PokeW_RAM(ULONG addr,UWORD data)=0;
  virtual UWORD   PeekW_RAM(ULONG addr)=0;
  
  virtual UBYTE*  GetRamPointer(void)=0;
};

#endif
