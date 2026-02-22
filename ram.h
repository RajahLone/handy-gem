#ifndef RAM_H
#define RAM_H

#define RAM_SIZE                                65536
#define RAM_ADDR_MASK                   0xffff
#define DEFAULT_RAM_CONTENTS    0xff

typedef struct
{
  UWORD   jump;
  UWORD   load_address;
  UWORD   size;
  UBYTE   magic[4];
}HOME_HEADER;

class CRam : public CLynxBase
{
  
  // Function members
  
public:
  CRam(UBYTE *filememory,ULONG filesize);
  ~CRam();
  
public:
  
  void    Reset(void);
  bool    ContextSave(FILE *fp);
  bool    ContextLoad(LSS_FILE *fp);
  
  void    Poke(ULONG addr, UBYTE data){ mRamData[addr]=data;};
  UBYTE   Peek(ULONG addr){ return(mRamData[addr]);};
  ULONG   ReadCycle(void) {return 5;};
  ULONG   WriteCycle(void) {return 5;};
  ULONG   ObjectSize(void) {return RAM_SIZE;};
  UBYTE*  GetRamPointer(void) { return mRamData; };
  
  // Data members
  
private:
  UBYTE   mRamData[RAM_SIZE];
  UBYTE   *mFileData;
  ULONG   mFileSize;
  
};

#endif
