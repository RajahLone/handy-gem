#define RAM_CPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "system.h"
#include "ram.h"

#include "amiga.h"

#define __min(a, b) (((a) < (b)) ? (a) : (b))

CRam::CRam(UBYTE *filememory,ULONG filesize)
{
  HOME_HEADER     header;
  
  // Take a copy into the backup buffer for restore on reset
  mFileSize=filesize;
  
  if(filesize)
  {
    // Take a copy of the ram data
    mFileData = new UBYTE[mFileSize];
    memcpy(mFileData,filememory,mFileSize);
    
    // Sanity checks on the header
    memcpy(&header,mFileData,sizeof(HOME_HEADER));
    
    if(header.magic[0]!='B' || header.magic[1]!='S' || header.magic[2]!='9' || header.magic[3]!='3')
    {   say((STRPTR) "File format invalid (magic number)!");
      throw 0;
    }
  }
  else
  {
    filememory=NULL;
  }
  // Reset will cause the loadup
  
  Reset();
}

CRam::~CRam()
{
  if(mFileSize)
  {
    delete[] mFileData;
    mFileData=NULL;
  }
}

void CRam::Reset(void)
{
  // Open up the file
  
  if(mFileSize >= sizeof(HOME_HEADER))
  {
    HOME_HEADER     header;
    UBYTE tmp;
    SLONG data_size;
    
    // Reverse the bytes in the header words
    memcpy(&header,mFileData,sizeof(HOME_HEADER));
    tmp=(header.load_address&0xff00)>>8;
    header.load_address=(header.load_address<<8)+tmp;
    tmp=(header.size&0xff00)>>8;
    header.size=(header.size<<8)+tmp;
    
    // Now we can safely read/manipulate the data
    header.load_address-=10;
    
    data_size = __min(SLONG(header.size), (SLONG)(mFileSize));
    memset(mRamData, 0x00, header.load_address);
    memcpy(mRamData+header.load_address, mFileData, data_size);
    memset(mRamData+header.load_address+data_size, 0x00, RAM_SIZE-header.load_address-data_size);
    gCPUBootAddress=header.load_address;
  } else {
    memset(mRamData, DEFAULT_RAM_CONTENTS, RAM_SIZE);
  }
}

bool CRam::ContextSave(FILE *fp)
{
  if(!fprintf(fp,"CRam::ContextSave")) return 0;
  if(!afwrite(mRamData,sizeof(UBYTE),RAM_SIZE,fp, 0)) return 0;
  return 1;
}

bool CRam::ContextLoad(LSS_FILE *fp)
{
  char teststr[100]="XXXXXXXXXXXXXXXXX";
  if(!lss_read(teststr,sizeof(char),17,fp, 0)) return 0;
  if(strcmp(teststr,"CRam::ContextSave")!=0) return 0;
  
  if(!lss_read(mRamData,sizeof(UBYTE),RAM_SIZE,fp, 0)) return 0;
  mFileSize=0;
  return 1;
}
