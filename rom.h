#ifndef ROM_H
#define ROM_H

#define ROM_SIZE              0x200
#define ROM_ADDR_MASK        0x01ff
#define DEFAULT_ROM_CONTENTS   0x88

#define BROM_START           0xfe00
#define BROM_SIZE             0x200
#define VECTOR_START         0xfffa
#define VECTOR_SIZE             0x6

#define MAX_PATH                512

class CRom : public CLynxBase
{   // Function members
  
public:
  CRom(char* romfile);
  
public:
  void  Reset(void);
  bool  ContextSave(FILE* fp);
  bool  ContextLoad(LSS_FILE* fp);
  
  void  Poke(ULONG addr, UBYTE data)
  {   if (mWriteEnable)
  {   mRomData[addr & ROM_ADDR_MASK] = data;
  }   };
  UBYTE Peek(ULONG addr)
  {   return(mRomData[addr & ROM_ADDR_MASK]);
  };
  ULONG ReadCycle(void)  { return 5;        };
  ULONG WriteCycle(void) { return 5;        };
  ULONG ObjectSize(void) { return ROM_SIZE; };
  
  // Data members
  
public:
  bool  mWriteEnable;
private:
  UBYTE mRomData[ROM_SIZE];
  char  mFileName[MAX_PATH + 1];
};

#endif
