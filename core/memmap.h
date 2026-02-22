#ifndef MEMMAP_H
#define MEMMAP_H

#define MEMMAP_SIZE                             0x1

#ifdef TRACE_CART

#define TRACE_MEMMAP0(msg)                                      _RPT1(_CRT_WARN,"CMamMap::"msg" (Time=%012d)\n",gSystemCycleCount)
#define TRACE_MEMMAP1(msg,arg1)                         _RPT2(_CRT_WARN,"CMamMap::"msg" (Time=%012d)\n",arg1,gSystemCycleCount)
#define TRACE_MEMMAP2(msg,arg1,arg2)            _RPT3(_CRT_WARN,"CMamMap::"msg" (Time=%012d)\n",arg1,arg2,gSystemCycleCount)
#define TRACE_MEMMAP3(msg,arg1,arg2,arg3)       _RPT4(_CRT_WARN,"CMamMap::"msg" (Time=%012d)\n",arg1,arg2,arg3,gSystemCycleCount)

#else

#define TRACE_MEMMAP0(msg)
#define TRACE_MEMMAP1(msg,arg1)
#define TRACE_MEMMAP2(msg,arg1,arg2)
#define TRACE_MEMMAP3(msg,arg1,arg2,arg3)

#endif

class CMemMap : public CLynxBase
{
  // Function members
  
public:
  CMemMap(CSystem& parent);
  
public:
  bool    ContextSave(FILE *fp);
  bool    ContextLoad(LSS_FILE *fp);
  void    Reset(void);
  
  void    Poke(ULONG addr,UBYTE data);
  UBYTE   Peek(ULONG addr);
  ULONG   ReadCycle(void) {return 5;};
  ULONG   WriteCycle(void) {return 5;};
  ULONG   ObjectSize(void) {return MEMMAP_SIZE;};
  
  // Data members
  
private:
  int                             mMikieEnabled;
  int                             mSusieEnabled;
  int                             mRomEnabled;
  int                             mVectorsEnabled;
  
  CSystem&                mSystem;
};

#endif
