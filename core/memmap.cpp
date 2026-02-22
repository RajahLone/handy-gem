#define MEMMAP_CPP

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "system.h"
#include "memmap.h"

CMemMap::CMemMap(CSystem& parent)
:mSystem(parent)
{   Reset();
}

void CMemMap::Reset(void)
{   // Initialise ALL pointers to RAM then overload to correct
  for(int loop=0;loop<SYSTEM_SIZE;loop++) mSystem.mMemoryHandlers[loop]=mSystem.mRam;
  
  // Special case for ourselves.
  mSystem.mMemoryHandlers[0xFFF8]=mSystem.mRam;
  mSystem.mMemoryHandlers[0xFFF9]=mSystem.mMemMap;
  
  mSusieEnabled=-1;
  mMikieEnabled=-1;
  mRomEnabled=-1;
  mVectorsEnabled=-1;
  
  // Initialise everything correctly
  Poke(0,0);
}

bool CMemMap::ContextSave(FILE *fp)
{   if(!fprintf(fp,"CMemMap::ContextSave")) return 0;
  if(!afwrite(&mMikieEnabled,   sizeof(ULONG), 1, fp, 4)) return 0;
  if(!afwrite(&mSusieEnabled,   sizeof(ULONG), 1, fp, 4)) return 0;
  if(!afwrite(&mRomEnabled,     sizeof(ULONG), 1, fp, 4)) return 0;
  if(!afwrite(&mVectorsEnabled, sizeof(ULONG), 1, fp, 4)) return 0;
  return 1;
}

bool CMemMap::ContextLoad(LSS_FILE *fp)
{   char teststr[100]="XXXXXXXXXXXXXXXXXXXX";
  
  // First put everything to a known state
  Reset();
  
  // Read back our parameters
  if(!lss_read(teststr,sizeof(char),20,fp, 0)) return 0;
  if(strcmp(teststr,"CMemMap::ContextSave")!=0) return 0;
  if(!lss_read(&mMikieEnabled,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mSusieEnabled,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mRomEnabled,sizeof(ULONG),1,fp, 4)) return 0;
  if(!lss_read(&mVectorsEnabled,sizeof(ULONG),1,fp, 4)) return 0;
  
  // The peek will give us the correct value to put back
  UBYTE mystate=Peek(0);
  
  // Now set to un-initialised so the poke will set correctly
  mSusieEnabled=-1;
  mMikieEnabled=-1;
  mRomEnabled=-1;
  mVectorsEnabled=-1;
  
  // Set banks correctly
  Poke(0,mystate);
  
  return 1;
}

inline void CMemMap::Poke(ULONG addr, UBYTE data)
{   TRACE_MEMMAP1("Poke() - Data %02x",data);
  
  int newstate,loop;
  
  // FC00-FCFF Susie area
  newstate=(data&0x01)?FALSE:TRUE;
  if(newstate!=mSusieEnabled)
  {   mSusieEnabled=newstate;
    
    if(mSusieEnabled)
    {   for(loop=SUSIE_START;loop<SUSIE_START+SUSIE_SIZE;loop++) mSystem.mMemoryHandlers[loop]=mSystem.mSusie;
    } else
    {   for(loop=SUSIE_START;loop<SUSIE_START+SUSIE_SIZE;loop++) mSystem.mMemoryHandlers[loop]=mSystem.mRam;
    }   }
  
  // FD00-FCFF Mikie area
  newstate=(data&0x02)?FALSE:TRUE;
  if(newstate!=mMikieEnabled)
  {   mMikieEnabled=newstate;
    
    if(mMikieEnabled)
    {   for(loop=MIKIE_START;loop<MIKIE_START+MIKIE_SIZE;loop++) mSystem.mMemoryHandlers[loop]=mSystem.mMikie;
    } else
    {   for(loop=MIKIE_START;loop<MIKIE_START+MIKIE_SIZE;loop++) mSystem.mMemoryHandlers[loop]=mSystem.mRam;
    }   }
  
  // FE00-FFF7 Rom area
  newstate=(data&0x04)?FALSE:TRUE;
  if(newstate!=mRomEnabled)
  {   mRomEnabled=newstate;
    
    if(mRomEnabled)
    {   for(loop=BROM_START;loop<BROM_START+(BROM_SIZE-8);loop++) mSystem.mMemoryHandlers[loop]=mSystem.mRom;
    } else
    {   for(loop=BROM_START;loop<BROM_START+(BROM_SIZE-8);loop++) mSystem.mMemoryHandlers[loop]=mSystem.mRam;
    }   }
  
  // FFFA-FFFF Vector area - Overload ROM space
  newstate=(data&0x08)?FALSE:TRUE;
  if(newstate!=mVectorsEnabled)
  {   mVectorsEnabled=newstate;
    
    if(mVectorsEnabled)
    {   for(loop=VECTOR_START;loop<VECTOR_START+VECTOR_SIZE;loop++) mSystem.mMemoryHandlers[loop]=mSystem.mRom;
    } else
    {   for(loop=VECTOR_START;loop<VECTOR_START+VECTOR_SIZE;loop++) mSystem.mMemoryHandlers[loop]=mSystem.mRam;
    }   }   }

inline UBYTE CMemMap::Peek(ULONG addr)
{   UBYTE retval=0;
  
  retval+=(mSusieEnabled)?0:0x01;
  retval+=(mMikieEnabled)?0:0x02;
  retval+=(mRomEnabled)?0:0x04;
  retval+=(mVectorsEnabled)?0:0x08;
  TRACE_MEMMAP1("Peek() - Data %02x",retval);
  return retval;
}
