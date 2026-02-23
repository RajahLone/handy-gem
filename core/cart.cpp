#define CART_CPP

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "system.h"
#include "cart.h"

#define __min(a, b) (((a) < (b)) ? (a) : (b))
#define __max(a, b) (((a) > (b)) ? (a) : (b))

#include "amiga.h"

MODULE ULONG crcTable[256];

CCart::CCart(UBYTE* gamedata, ULONG gamesize)
{       LYNX_HEADER header;
  
  mWriteEnableBank0=FALSE;
  mWriteEnableBank1=FALSE;
  mCartRAM=FALSE;
  mHeaderLess=FALSE;
  /* mCRC32=0;
   mCRC32=crc32(mCRC32,gamedata,gamesize); */
  
  // Open up the file
  
  if(gamesize)
  {
    
    // Checkout the header bytes
    memcpy(&header,gamedata,sizeof(LYNX_HEADER));
    
    header.page_size_bank0 = ((header.page_size_bank0>>8) | (header.page_size_bank0<<8));
    header.page_size_bank1 = ((header.page_size_bank1>>8) | (header.page_size_bank1<<8));
    header.version         = ((header.version>>8) | (header.version<<8));
    
    // Sanity checks on the header
    
    if(header.magic[0]!='L' || header.magic[1]!='Y' || header.magic[2]!='N' || header.magic[3]!='X' || header.version!=1)
    {
      say((STRPTR) "File format invalid (magic number)!");
      throw 0;
    }
    
    // Setup name & manufacturer
    
    strncpy(mName,(char*)&header.cartname,32);;
    strncpy(mManufacturer,(char*)&header.manufname,16);
    
    // Setup rotation
    
    mRotation=header.rotation;
    if(mRotation!=CART_NO_ROTATE && mRotation!=CART_ROTATE_LEFT && mRotation!=CART_ROTATE_RIGHT) mRotation=CART_NO_ROTATE;
  }
  else
  {
    header.page_size_bank0=0x000;
    header.page_size_bank1=0x000;
    
    // Setup name & manufacturer
    
    strcpy(mName,"<No cart loaded>");
    strcpy(mManufacturer,"<No cart loaded>");
    
    // Setup rotation
    
    mRotation=CART_NO_ROTATE;
  }
  
  // Set the filetypes
  
  CTYPE banktype0,banktype1;
  
  switch (header.page_size_bank0)
  {
    case 0x000:
      banktype0    = ZUNUSED;
      mMaskBank0   = 0;
      mShiftCount0 = 0;
      mCountMask0  = 0;
    case 0x100:
      banktype0    = C64K;
      mMaskBank0   = 0x00ffff;
      mShiftCount0 = 8;
      mCountMask0  = 0x0ff;
    case 0x200:
      banktype0    = C128K;
      mMaskBank0   = 0x01ffff;
      mShiftCount0 = 9;
      mCountMask0  = 0x1ff;
    case 0x400:
      banktype0    = C256K;
      mMaskBank0   = 0x03ffff;
      mShiftCount0 = 10;
      mCountMask0  = 0x3ff;
    case 0x800:
      banktype0    = C512K;
      mMaskBank0   = 0x07ffff;
      mShiftCount0 = 11;
      mCountMask0  = 0x7ff;
    default:
      say((STRPTR) "File format invalid (bank 0)!");
      throw 0;
  }
  
  switch(header.page_size_bank1)
  {
    case 0x000:
      banktype1=ZUNUSED;
      mMaskBank1=0;
      mShiftCount1=0;
      mCountMask1=0;
      break;
    case 0x100:
      banktype1=C64K;
      mMaskBank1=0x00ffff;
      mShiftCount1=8;
      mCountMask1=0x0ff;
      break;
    case 0x200:
      banktype1=C128K;
      mMaskBank1=0x01ffff;
      mShiftCount1=9;
      mCountMask1=0x1ff;
      break;
    case 0x400:
      banktype1=C256K;
      mMaskBank1=0x03ffff;
      mShiftCount1=10;
      mCountMask1=0x3ff;
      break;
    case 0x800:
      banktype1=C512K;
      mMaskBank1=0x07ffff;
      mShiftCount1=11;
      mCountMask1=0x7ff;
      break;
    default:
      say((STRPTR) "File format invalid (bank 1)!");
      throw 0;
  }
  
  // Make some space for the new carts
  
  mCartBank0 = (UBYTE*) new UBYTE[mMaskBank0+1];
  mCartBank1 = (UBYTE*) new UBYTE[mMaskBank1+1];
  
  // Set default bank
  
  mBank=bank0;
  
  // Initialiase
  
  int cartsize = __max(0, int(gamesize - sizeof(LYNX_HEADER)));
  int bank0size = __min(cartsize, (int)(mMaskBank0+1));
  int bank1size = __min(cartsize, (int)(mMaskBank1+1));
  memcpy(
         mCartBank0,
         gamedata+(sizeof(LYNX_HEADER)),
         bank0size);
  memset(
         mCartBank0 + bank0size,
         DEFAULT_CART_CONTENTS,
         mMaskBank0+1 - bank0size);
  memcpy(
         mCartBank1,
         gamedata+(sizeof(LYNX_HEADER)),
         bank1size);
  memset(
         mCartBank1 + bank0size,
         DEFAULT_CART_CONTENTS,
         mMaskBank1+1 - bank1size);
  
  // Copy the cart banks from the image
  if (gamesize)
  {   // As this is a cartridge boot unset the boot address
    
    gCPUBootAddress = 0;
    
    //
    // Check if this is a headerless cart
    //
    mHeaderLess = TRUE;
    for (int loop = 0; loop < 32; loop++)
    {   if (mCartBank0[loop&mMaskBank0] != 0x00) mHeaderLess = FALSE;
    }   }
  
  // Dont allow an empty Bank1 - Use it for shadow SRAM/EEPROM
  if(banktype1==ZUNUSED)
  {
    // Delete the single byte allocated  earlier
    delete[] mCartBank1;
    // Allocate some new memory for us
    banktype1=C64K;
    mMaskBank1=0x00ffff;
    mShiftCount1=8;
    mCountMask1=0x0ff;
    mCartBank1 = (UBYTE*) new UBYTE[mMaskBank1+1];
    memset(mCartBank1, DEFAULT_RAM_CONTENTS, mMaskBank1+1);
    mWriteEnableBank1=TRUE;
    mCartRAM=TRUE;
  }
}

CCart::~CCart()
{   delete[] mCartBank0;
  delete[] mCartBank1;
}

void CCart::Reset(void)
{   mCounter=0;
  mShifter=0;
  mAddrData=0;
  mStrobe=0;
}

bool CCart::ContextSave(FILE *fp)
{   // assert(sizeof(EMMODE) == 4);
  
  if(!fprintf(fp,"CCart::ContextSave")) return 0;
  if(!afwrite(&mCounter,          sizeof(ULONG),  1, fp, 4)) return 0;
  if(!afwrite(&mShifter,          sizeof(ULONG),  1, fp, 4)) return 0;
  if(!afwrite(&mAddrData,         sizeof(ULONG),  1, fp, 4)) return 0;
  if(!afwrite(&mStrobe,           sizeof(ULONG),  1, fp, 4)) return 0;
  if(!afwrite(&mShiftCount0,      sizeof(ULONG),  1, fp, 4)) return 0;
  if(!afwrite(&mCountMask0,       sizeof(ULONG),  1, fp, 4)) return 0;
  if(!afwrite(&mShiftCount1,      sizeof(ULONG),  1, fp, 4)) return 0;
  if(!afwrite(&mCountMask1,       sizeof(ULONG),  1, fp, 4)) return 0;
  if(!afwrite(&mBank,             sizeof(EMMODE), 1, fp, 4)) return 0;
  if(!afwrite(&mWriteEnableBank0, sizeof(ULONG),  1, fp, 4)) return 0;
  if(!afwrite(&mWriteEnableBank1, sizeof(ULONG),  1, fp, 4)) return 0;
  if(!afwrite(&mCartRAM,          sizeof(ULONG),  1, fp, 4)) return 0;
  if(mCartRAM)
  {   if(!afwrite(&mMaskBank1,    sizeof(ULONG),  1, fp, 4)) return 0;
    if(!afwrite(mCartBank1,     sizeof(UBYTE),  mMaskBank1+1, fp, 0)) return 0;
  }
  return 1;
}

bool CCart::ContextSaveLegacy(FILE *fp)
{   // assert(sizeof(EMMODE) == 4);
  
  if(!fprintf(fp,"CCart::ContextSave")) return 0;
  if(!afwrite(&mRotation,sizeof(ULONG),1,fp, 4)) return 0;
  if(!afwrite(&mHeaderLess,sizeof(ULONG),1,fp, 4)) return 0;
  if(!afwrite(&mCounter,sizeof(ULONG),1,fp, 4)) return 0;
  if(!afwrite(&mShifter,sizeof(ULONG),1,fp, 4)) return 0;
  if(!afwrite(&mAddrData,sizeof(ULONG),1,fp, 4)) return 0;
  if(!afwrite(&mStrobe,sizeof(ULONG),1,fp, 4)) return 0;
  if(!afwrite(&mShiftCount0,sizeof(ULONG),1,fp, 4)) return 0;
  if(!afwrite(&mCountMask0,sizeof(ULONG),1,fp, 4)) return 0;
  if(!afwrite(&mShiftCount1,sizeof(ULONG),1,fp, 4)) return 0;
  if(!afwrite(&mCountMask1,sizeof(ULONG),1,fp, 4)) return 0;
  // assert(sizeof(EMMODE) == 4);
  if(!afwrite(&mBank,sizeof(EMMODE),1,fp, 4)) return 0;
  
  if(!afwrite(&mMaskBank0,sizeof(ULONG),1,fp, 4)) return 0;
  if(!afwrite(&mMaskBank1,sizeof(ULONG),1,fp, 4)) return 0;
  
  if(!afwrite(mCartBank0,sizeof(UBYTE),mMaskBank0+1,fp, 0)) return 0;
  if(!afwrite(mCartBank1,sizeof(UBYTE),mMaskBank1+1,fp, 0)) return 0;
  return 1;
}

bool CCart::ContextLoad(LSS_FILE *fp)
{       char teststr[100]="XXXXXXXXXXXXXXXXXX";
  
  if(!lss_read(teststr,sizeof(char),18,fp, 0)) return 0;
  if(strcmp(teststr,"CCart::ContextSave")!=0) return 0;
  if(!lss_read(&mCounter,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mShifter,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mAddrData,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mStrobe,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mShiftCount0,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mCountMask0,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mShiftCount1,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mCountMask1,sizeof(ULONG),1,fp, 4)) return 0;
  // assert(sizeof(EMMODE) == 4);
  if(!lss_read(&mBank,sizeof(EMMODE),1,fp, 4)) return 0;
  if(!lss_read(&mWriteEnableBank0,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mWriteEnableBank1,sizeof(ULONG),1,fp, 4)) return 0;
  
  if(!lss_read(&mCartRAM,sizeof(ULONG),1,fp, 4)) return 0;
  if(mCartRAM)
  {
    if(!lss_read(&mMaskBank1,sizeof(ULONG),1,fp, 4)) return 0;
    delete[] mCartBank1;
    mCartBank1 = new UBYTE[mMaskBank1+1];
    if(!lss_read(mCartBank1,sizeof(UBYTE),mMaskBank1+1,fp, 0)) return 0;
  }
  
  return 1;
}

bool CCart::ContextLoadLegacy(LSS_FILE *fp)
{   strcpy(mName,"<** IMAGE **>");
  strcpy(mManufacturer,"<** RESTORED **>");
  char teststr[100]="XXXXXXXXXXXXXXXXXX";
  
  if(!lss_read(teststr,sizeof(char),18,fp, 0)) return 0;
  if(strcmp(teststr,"CCart::ContextSave")!=0) return 0;
  if(!lss_read(&mRotation,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mHeaderLess,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mCounter,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mShifter,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mAddrData,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mStrobe,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mShiftCount0,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mCountMask0,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mShiftCount1,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mCountMask1,sizeof(ULONG),1,fp, 4)) return 0;
  // assert(sizeof(EMMODE) == 4);
  if(!lss_read(&mBank,sizeof(EMMODE),1,fp, 4)) return 0;
  
  if(!lss_read(&mMaskBank0,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mMaskBank1,sizeof(ULONG),1,fp, 4)) return 0;
  
  delete[] mCartBank0;
  delete[] mCartBank1;
  mCartBank0 = new UBYTE[mMaskBank0+1];
  mCartBank1 = new UBYTE[mMaskBank1+1];
  if(!lss_read(mCartBank0,sizeof(UBYTE),mMaskBank0+1,fp, 0)) return 0;
  if(!lss_read(mCartBank1,sizeof(UBYTE),mMaskBank1+1,fp, 0)) return 0;
  
  /* mCRC32 = 0;
   mCRC32 = crc32(mCRC32, ?, ?); */
  
  return 1;
}

inline void CCart::Poke(ULONG addr, UBYTE data)
{
  if(mBank==bank0)
  {
    if(mWriteEnableBank0) mCartBank0[addr&mMaskBank0]=data;
  }
  else
  {
    if(mWriteEnableBank1) mCartBank1[addr&mMaskBank1]=data;
  }
}

inline UBYTE CCart::Peek(ULONG addr)
{
  if(mBank==bank0)
  {
    return(mCartBank0[addr&mMaskBank0]);
  }
  else
  {
    return(mCartBank1[addr&mMaskBank1]);
  }
}

void CCart::CartAddressStrobe(bool strobe)
{
  static int last_strobe=0;
  
  mStrobe=strobe;
  
  if(mStrobe) mCounter=0;
  
  //
  // Either of the two below seem to work OK.
  //
  // if(!strobe && last_strobe)
  //
  if(mStrobe && !last_strobe)
  {
    // Clock a bit into the shifter
    mShifter=mShifter<<1;
    mShifter+=mAddrData?1:0;
    mShifter&=0xff;
  }
  last_strobe=mStrobe;
}

void CCart::CartAddressData(bool data)
{   mAddrData = data;
}

void CCart::Poke0(UBYTE data)
{
  if(mWriteEnableBank0)
  {
    ULONG address=(mShifter<<mShiftCount0)+(mCounter&mCountMask0);
    mCartBank0[address&mMaskBank0]=data;
  }
  if(!mStrobe)
  {
    mCounter++;
    mCounter&=0x07ff;
  }
}

void CCart::Poke1(UBYTE data)
{
  if(mWriteEnableBank1)
  {
    ULONG address=(mShifter<<mShiftCount1)+(mCounter&mCountMask1);
    mCartBank1[address&mMaskBank1]=data;
  }
  if(!mStrobe)
  {
    mCounter++;
    mCounter&=0x07ff;
  }
}

UBYTE CCart::Peek0(void)
{
  ULONG address=(mShifter<<mShiftCount0)+(mCounter&mCountMask0);
  UBYTE data=mCartBank0[address&mMaskBank0];
  
  if(!mStrobe)
  {
    mCounter++;
    mCounter&=0x07ff;
  }
  
  return data;
}

UBYTE CCart::Peek1(void)
{
  ULONG address=(mShifter<<mShiftCount1)+(mCounter&mCountMask1);
  UBYTE data=mCartBank1[address&mMaskBank1];
  
  if(!mStrobe)
  {
    mCounter++;
    mCounter&=0x07ff;
  }
  
  return data;
}

/*
 uLong crc32(uLong crc, const Bytef* buf, uInt len)
 {   return getcrc32((unsigned char*) buf, len);
 }
 
 ULONG getcrc32(UBYTE* address, int thesize)
 {   register ULONG crc;
 int   i;
 
 crc = 0xFFFFFFFF;
 for (i = 0; i < thesize; i++)
 {   crc = ((crc >> 8) & 0x00FFFFFF) ^ crcTable[(crc ^ *address++) & 0xFF];
 }
 return crc ^ 0xFFFFFFFF;
 }
 */

EXPORT void generate_crctable(void)
{   ULONG crc;
  int   i, j;
  
  for (i = 0; i < 256; i++)
  {   crc = (ULONG) i;
    for (j = 8; j > 0; j--)
    {   if (crc & 1)
    {   crc = (crc >> 1) ^ 0xEDB88320L;
    } else
    {   crc >>= 1;
    }   }
    crcTable[i] = crc;
  }   }
