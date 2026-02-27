#define SYSTEM_CPP

typedef char* STRPTR;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "system.h"

#include <ctype.h>
#include <cstdio>

#include "amiga.h"
#include "../appl/main.h"


CSystem::CSystem(char* gamefile,char* romfile)
:mCart(NULL),
mRom(NULL),
mMemMap(NULL),
mRam(NULL),
mCpu(NULL),
mMikie(NULL),
mSusie(NULL)
{
  
  // Select the default filetype
  UBYTE *filememory=NULL;
  ULONG filesize=0;
  
  mFileType=HANDY_FILETYPE_LNX;
  if(strcmp(gamefile,"")==0)
  {
    // No file
    filesize=0;
    filememory=NULL;
  } else
  {
    // Open the file and load the file
    FILE    *fp;
    
    // Open the cartridge file for reading
    if((fp=fopen(gamefile,"rb"))==NULL)
    {   say((STRPTR) "File not found!");
      throw 0;
    }
    
    // How big is the file ??
    fseek(fp,0,SEEK_END);
    filesize=ftell(fp);
    fseek(fp,0,SEEK_SET);
    filememory=(UBYTE*) new UBYTE[filesize];
    
    if(fread(filememory,sizeof(char),filesize,fp)!=filesize)
    {   delete filememory;
      say((STRPTR) "File read error!");
      throw 0;
    }
    
    fclose(fp);
  }
  
  // Now try and determine the filetype we have opened
  if(filesize)
  {
    char clip[11];
    memcpy(clip,filememory,11);
    clip[4]=0;
    clip[10]=0;
    
    if(!strcmp(&clip[6],"BS93")) mFileType=HANDY_FILETYPE_HOMEBREW;
    else if(!strcmp(&clip[0],"LYNX")) mFileType=HANDY_FILETYPE_LNX;
    else if(!strcmp(&clip[0],LSS_VERSION_OLD)) mFileType=HANDY_FILETYPE_SNAPSHOT;
    else
    {   delete filememory;
      mFileType = HANDY_FILETYPE_ILLEGAL;
      say((STRPTR) "Unsupported file format (unrecognized)!");
      throw 0;
    }   }
  
  mCycleCountBreakpoint=0xffffffff;
  
  // Create the system objects that we'll use
  
  // Attempt to load the cartridge errors caught above here...
  
  mRom = new CRom(romfile);
  
  // An exception from this will be caught by the level above
  
  switch(mFileType)
  {
    case HANDY_FILETYPE_LNX:
      mCart = new CCart(filememory,filesize);
      if(mCart->CartHeaderLess())
      {   say((STRPTR) "Unsupported file format (headerless)!");
        throw 0;
      }
      else
      {
        mRam = new CRam(0,0);
      }
      break;
    case HANDY_FILETYPE_HOMEBREW:
      say((STRPTR) "Unsupported file format (homebrew)!");
      throw 0;
      break;
    case HANDY_FILETYPE_SNAPSHOT:
    case HANDY_FILETYPE_ILLEGAL:
    default:
      mCart = new CCart(0,0);
      mRam = new CRam(0,0);
  }
  
  // These can generate exceptions
  
  mMikie = new CMikie(*this);
  mSusie = new CSusie(*this);
  
  // Instantiate the memory map handler
  
  mMemMap = new CMemMap(*this);
  
  // Now the handlers are set we can instantiate the CPU as is will use handlers on reset
  
  mCpu = new C65C02(*this);
  
  // Now init is complete do a reset, this will cause many things to be reset twice
  // but what the hell, who cares, I don't.....
  
  Reset();
  
  // If this is a snapshot type then restore the context
  
  if(mFileType==HANDY_FILETYPE_SNAPSHOT)
  {
    if(!ContextLoad(gamefile))
    {   Reset();
      say((STRPTR) "State load error!");
      throw 0;
    }
  }
  if(filesize) delete filememory;
}

CSystem::~CSystem()
{
  // Cleanup all our objects
  
  if(mCart!=NULL) delete mCart;
  if(mRom!=NULL) delete mRom;
  if(mRam!=NULL) delete mRam;
  if(mCpu!=NULL) delete mCpu;
  if(mMikie!=NULL) delete mMikie;
  if(mSusie!=NULL) delete mSusie;
  if(mMemMap!=NULL) delete mMemMap;
}

void CSystem::Reset(void)
{
  gSystemCycleCount=0;
  gNextTimerEvent=0;
  gCPUBootAddress=0;
  gBreakpointHit=FALSE;
  gSingleStepMode=FALSE;
  gSingleStepModeSprites=FALSE;
  gSystemIRQ=FALSE;
  gSystemNMI=FALSE;
  gSystemCPUSleep=FALSE;
  gSystemHalt=FALSE;
  
  gThrottleLastTimerCount=0;
  gThrottleNextCycleCheckpoint=0;
  
  gTimerCount=0;
  
  gAudioBufferPointer=0;
  gAudioLastUpdateCycle=0;
  memset(gAudioBuffer,128,HANDY_AUDIO_BUFFER_SIZE);
  
  mMemMap->Reset();
  mCart->Reset();
  mRom->Reset();
  mRam->Reset();
  mMikie->Reset();
  mSusie->Reset();
  mCpu->Reset();
  
  // Homebrew hashup
  
  if(mFileType==HANDY_FILETYPE_HOMEBREW)
  {
    mMikie->PresetForHomebrew();
    
    C6502_REGS regs;
    mCpu->GetRegs(regs);
    regs.PC=(UWORD)gCPUBootAddress;
    mCpu->SetRegs(regs);
  }
}

bool CSystem::ContextSave(char *context, int kind)
{
  FILE *fp;
  bool status=1;
  
  if((fp=fopen(context,"wb"))==NULL) return false;
  
  if (kind == 0)
  {   if(!fprintf(fp,LSS_VERSION_OLD)) status=0;
  } else
  {   if(!fprintf(fp,LSS_VERSION)) status=0;
    
    // Save ROM CRC
    ULONG checksum=mCart->CRC32();
    if(!afwrite(&checksum,sizeof(ULONG),1,fp, 4)) status=0;
  }
  
  if(!fprintf(fp,"CSystem::ContextSave")) status=0;
  
  if(!afwrite(&mCycleCountBreakpoint,        sizeof(ULONG), 1, fp, 4)) status=0;
  if(!afwrite(&gSystemCycleCount,            sizeof(ULONG), 1, fp, 4)) status=0;
  if(!afwrite(&gNextTimerEvent,              sizeof(ULONG), 1, fp, 4)) status=0;
  if(!afwrite(&gCPUWakeupTime,               sizeof(ULONG), 1, fp, 4)) status=0;
  if(!afwrite(&gCPUBootAddress,              sizeof(ULONG), 1, fp, 4)) status=0;
  if(!afwrite(&gIRQEntryCycle,               sizeof(ULONG), 1, fp, 4)) status=0;
  if(!afwrite(&gBreakpointHit,               sizeof(ULONG), 1, fp, 4)) status=0;
  if(!afwrite(&gSingleStepMode,              sizeof(ULONG), 1, fp, 4)) status=0;
  if(!afwrite(&gSystemIRQ,                   sizeof(ULONG), 1, fp, 4)) status=0;
  if(!afwrite(&gSystemNMI,                   sizeof(ULONG), 1, fp, 4)) status=0;
  if(!afwrite(&gSystemCPUSleep,              sizeof(ULONG), 1, fp, 4)) status=0;
  if(!afwrite(&gSystemCPUSleep_Saved,        sizeof(ULONG), 1, fp, 4)) status=0;
  if(!afwrite(&gSystemHalt,                  sizeof(ULONG), 1, fp, 4)) status=0;
  if(!afwrite(&gThrottleMaxPercentage,       sizeof(ULONG), 1, fp, 4)) status=0;
  if(!afwrite(&gThrottleLastTimerCount,      sizeof(ULONG), 1, fp, 4)) status=0;
  if(!afwrite(&gThrottleNextCycleCheckpoint, sizeof(ULONG), 1, fp, 4)) status=0;
  
  ULONG tmp=gTimerCount;
  if(!afwrite(&tmp,                          sizeof(ULONG), 1, fp, 4)) status=0;
  
  if(!afwrite(gAudioBuffer,                  sizeof(UBYTE), HANDY_AUDIO_BUFFER_SIZE, fp, 0)) status=0;
  if(!afwrite(&gAudioBufferPointer,          sizeof(ULONG), 1, fp, 4)) status=0;
  if(!afwrite(&gAudioLastUpdateCycle,        sizeof(ULONG), 1, fp, 4)) status=0;
  
  // Save other device contexts
  if(!mMemMap->ContextSave(fp)) status=0;
  if (kind == 0)
  {   if(!mCart->ContextSaveLegacy(fp)) status=0;
    if(!mRom->ContextSave(fp)) status=0;
  } else
  {   if(!mCart->ContextSave(fp)) status=0;
  }
  if(!mRam->ContextSave(fp)) status=0;
  if(!mMikie->ContextSave(fp)) status=0;
  if(!mSusie->ContextSave(fp)) status=0;
  if(!mCpu->ContextSave(fp)) status=0;
  
  fclose(fp);
  return status;
}

bool CSystem::ContextLoad(char *context)
{
  LSS_FILE *fp;
  bool status=1;
  UBYTE *filememory=NULL;
  ULONG filesize=0;
  FILE *fp1;
  
  if((fp1=fopen(context,"rb"))==NULL) status=0;
  
  fseek(fp1,0,SEEK_END);
  filesize=ftell(fp1);
  fseek(fp1,0,SEEK_SET);
  filememory=(UBYTE*) new UBYTE[filesize];
  
  if(fread(filememory,sizeof(char),filesize,fp1)!=filesize)
  {
    fclose(fp1);
    return 1;
  }
  fclose(fp1);
  
  // Setup our read structure
  fp = new LSS_FILE;
  fp->memptr=filememory;
  fp->index=0;
  fp->index_limit=filesize;
  
  char teststr[100];
  // Check identifier
  if(!lss_read(teststr,sizeof(char),4,fp, 0)) status=0;
  teststr[4]=0;
  
  if(strcmp(teststr,LSS_VERSION)==0 || strcmp(teststr,LSS_VERSION_OLD)==0)
  {
    bool legacy=FALSE;
    if(strcmp(teststr,LSS_VERSION_OLD)==0)
    {
      legacy=TRUE;
    }
    else
    {
      ULONG checksum;
      // Read CRC32 and check against the CART for a match
      lss_read(&checksum,sizeof(ULONG),1,fp,4);
      /* if(mCart->CRC32()!=checksum)
       {
       delete fp;
       delete filememory;
       printf("LSS Snapshot CRC does not match the loaded cartridge image, aborting load");
       return 0;
       } */
    }
    
    // Check our block header
    if(!lss_read(teststr,sizeof(char),20,fp,0)) status=0;
    teststr[20]=0;
    if(strcmp(teststr,"CSystem::ContextSave")!=0) status=0;
    
    if(!lss_read(&mCycleCountBreakpoint,sizeof(ULONG),1,fp, 4)) status=0;
    if(!lss_read(&gSystemCycleCount,sizeof(ULONG),1,fp, 4)) status=0;
    if(!lss_read(&gNextTimerEvent,sizeof(ULONG),1,fp, 4)) status=0;
    if(!lss_read(&gCPUWakeupTime,sizeof(ULONG),1,fp, 4)) status=0;
    if(!lss_read(&gCPUBootAddress,sizeof(ULONG),1,fp, 4)) status=0;
    if(!lss_read(&gIRQEntryCycle,sizeof(ULONG),1,fp, 4)) status=0;
    if(!lss_read(&gBreakpointHit,sizeof(ULONG),1,fp, 4)) status=0;
    if(!lss_read(&gSingleStepMode,sizeof(ULONG),1,fp, 4)) status=0;
    if(!lss_read(&gSystemIRQ,sizeof(ULONG),1,fp, 4)) status=0;
    if(!lss_read(&gSystemNMI,sizeof(ULONG),1,fp, 4)) status=0;
    if(!lss_read(&gSystemCPUSleep,sizeof(ULONG),1,fp, 4)) status=0;
    if(!lss_read(&gSystemCPUSleep_Saved,sizeof(ULONG),1,fp, 4)) status=0;
    if(!lss_read(&gSystemHalt,sizeof(ULONG),1,fp, 4)) status=0;
    if(!lss_read(&gThrottleMaxPercentage,sizeof(ULONG),1,fp, 4)) status=0;
    if(!lss_read(&gThrottleLastTimerCount,sizeof(ULONG),1,fp, 4)) status=0;
    if(!lss_read(&gThrottleNextCycleCheckpoint,sizeof(ULONG),1,fp, 4)) status=0;
    
    ULONG tmp;
    if(!lss_read(&tmp,sizeof(ULONG),1,fp,4)) status=0;
    gTimerCount=tmp;
    
    if(!lss_read(gAudioBuffer,sizeof(UBYTE),HANDY_AUDIO_BUFFER_SIZE,fp, 0)) status=0;
    if(!lss_read(&gAudioBufferPointer,sizeof(ULONG),1,fp, 4)) status=0;
    if(!lss_read(&gAudioLastUpdateCycle,sizeof(ULONG),1,fp, 4)) status=0;
    
    if(!mMemMap->ContextLoad(fp)) status=0;
    // Legacy support
    if(legacy)
    {
      if(!mCart->ContextLoadLegacy(fp)) status=0;
      if(!mRom->ContextLoad(fp)) status=0;
    }
    else
    {
      if(!mCart->ContextLoad(fp)) status=0;
    }
    if(!mRam->ContextLoad(fp)) status=0;
    if(!mMikie->ContextLoad(fp)) status=0;
    if(!mSusie->ContextLoad(fp)) status=0;
    if(!mCpu->ContextLoad(fp)) status=0;
  } else
  {   printf("Not a recognised LSS file\n");
  }
  
  delete fp;
  delete filememory;
  
  return status;
}
