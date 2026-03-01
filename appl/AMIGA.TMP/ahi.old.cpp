#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dos/dostags.h>
#include <exec/exec.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <devices/ahi.h>
#include <devices/audio.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <clib/alib_protos.h>

#include "main.h"

/* sample functions */
void ahi_set_volume(s32 volume);
void ahi_set_position(s32 position);
s32 ahi_samples_to_bytes(s32 samples);
s32 ahi_bytes_to_samples(s32 bytes);
s32 ahi_samples_buffered(void);
s32 ahi_samples_free(void);
void ahi_pause(void);

/* byte functions */
#define ahi_play_bytes(data, bytes, time, wait) ahi_play_samples(data, ahi_bytes_to_samples(bytes), time, wait)
#define ahi_bytes_buffered()                    ahi_samples_to_bytes(ahi_samples_buffered())
#define ahi_bytes_free()                        ahi_samples_to_bytes(ahi_samples_free())

#define ahi_task_name "AHIAudioServerTask"

#if 1
#define PRINTF(a...)
#define DEBUG(a...)
#else
#define PRINTF(a...) printf(a)
#define DEBUG(a...) printf(a)
#endif

struct timer_s
{
  struct MsgPort*     TimerMP;
  struct timerequest* TimerIO;
  struct Device*      TimerBase;
};

typedef struct audio_buffer_s
{
  void* buffer;
  s32   size;
  s64   time;
  s32   used;
} audio_buffer_t;

typedef struct audio_s {
  s32 frequency;
  s32 volume;
  s32 position;
  u32 mode;
  s32 fragsize;
  s32 frags;
  audio_buffer_t* audio_buffers;
  s64 current_time;
  s32 read_buffer;
  s32 write_buffer;
  void (*audio_sync)(s64 time);
  
  s32 play;
  s32 samples_to_bytes;
  
  s32 read_position;
  struct timeval tv;
  s32 write_position;
  
  struct Task* main_task;
  struct Task* audio_task;
  struct SignalSemaphore* semaphore;
  struct Process* audio_process;
  struct timer_s* timer;
} audio_t;

static audio_t audio;

/* to task:
 * SIGBREAKF_CTRL_C -
 * SIGBREAKF_CTRL_D wake up
 * SIGBREAKF_CTRL_E exit
 * SIGBREAKF_CTRL_F -
 *
 * from task:
 * SIGBREAKF_CTRL_C -
 * SIGBREAKF_CTRL_D wake up
 * SIGBREAKF_CTRL_E error
 * SIGBREAKF_CTRL_F -
 */

#define ahi_task_name "AHIAudioServerTask"

#define FLAG_EXIT (1 << 0)

#ifdef __amigaos4__
IMPORT struct Device*     TimerBase;
IMPORT struct TimerIFace* ITimer;
#endif

struct timer_s* timer_init(void);
void timer_gettime(struct timer_s* timer, struct timeval* tv);
void timer_subtime(struct timer_s* timer, struct timeval* dt, struct timeval* st);
void timer_usleep(struct timer_s* timer, int us);
void timer_exit(struct timer_s* timer);

static void ahi_task(void)
{
  struct AHIRequest *AHIIO  = NULL,
  *AHIIO1 = NULL,
  *AHIIO2 = NULL;
  struct MsgPort    *AHIMP1 = NULL,
  *AHIMP2 = NULL;
  u32                signals, flags;
  s32                device, used;
  struct AHIRequest* link   = NULL;
  struct timer_s*    timer  = NULL;
  s32                previous_read_buffer = -1;
  
  flags = 0;
  signals = 0;
  device = 1;
  
  if ((timer = timer_init()))
  {   if ((AHIMP1 = CreateMsgPort()))
  {   if ((AHIMP2 = CreateMsgPort()))
  {   if ((AHIIO1 = (struct AHIRequest*) CreateIORequest(AHIMP1, sizeof(struct AHIRequest))))
  {   if ((AHIIO2 = (struct AHIRequest*) CreateIORequest(AHIMP2, sizeof(struct AHIRequest))))
  {    device = OpenDevice(AHINAME, 0, (struct IORequest*) AHIIO1, 0);
  }   }   }   }   }
  
  if (device == 0)
  {   *AHIIO2 = *AHIIO1;
    AHIIO1->ahir_Std.io_Message.mn_ReplyPort = AHIMP1;
    AHIIO2->ahir_Std.io_Message.mn_ReplyPort = AHIMP2;
    AHIIO1->ahir_Std.io_Message.mn_Node.ln_Pri = 127;
    AHIIO2->ahir_Std.io_Message.mn_Node.ln_Pri = 127;
    AHIIO = AHIIO1;
    
    Signal(audio.main_task, SIGBREAKF_CTRL_D);
    
    for (;;)
    {   if (flags & FLAG_EXIT)
    {   break;
    }
      
      ObtainSemaphore(audio.semaphore);
      used = audio.audio_buffers[audio.read_buffer].used;
      ReleaseSemaphore(audio.semaphore);
      
      if (audio.play & used)
      {
        AHIIO->ahir_Std.io_Command = CMD_WRITE;
        AHIIO->ahir_Std.io_Data    = audio.audio_buffers[audio.read_buffer].buffer;
        AHIIO->ahir_Std.io_Length  = audio.audio_buffers[audio.read_buffer].size;
        AHIIO->ahir_Std.io_Offset  = 0;
        AHIIO->ahir_Frequency      = audio.frequency;
        AHIIO->ahir_Type           = audio.mode;
        AHIIO->ahir_Volume         = audio.volume;
        AHIIO->ahir_Position       = audio.position;
        AHIIO->ahir_Link           = link;
        
        SendIO((struct IORequest*) AHIIO);
        
        if (link)
        {
          u32 sigbit = 1 << link->ahir_Std.io_Message.mn_ReplyPort->mp_SigBit;
          
          for (;;)
          {
            signals = Wait(SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_D | sigbit);
            
            if (signals & SIGBREAKF_CTRL_E)
            {   flags |= FLAG_EXIT;
            }
            
            /* break when we got what we want */
            if (signals & sigbit)
            {   break;
            }   }
          
          WaitIO((struct IORequest*) link);
        }
        
        ObtainSemaphore(audio.semaphore);
        
        if (previous_read_buffer >= 0)
        {   audio.audio_buffers[previous_read_buffer].used = 0;
        }
        
        audio.read_position = (audio.read_buffer * audio.fragsize);
        timer_gettime(timer, &audio.tv);
        
        link = AHIIO;
        
        if (AHIIO == AHIIO1)
        {   AHIIO = AHIIO2;
        } else
        {   AHIIO = AHIIO1;
        }
        
        previous_read_buffer = audio.read_buffer;
        audio.read_buffer++;
        if (audio.read_buffer >= audio.frags)
        {   audio.read_buffer = 0;
        }
        
        ReleaseSemaphore(audio.semaphore);
        
        if (audio.audio_sync)
        {   audio.audio_sync(audio.audio_buffers[previous_read_buffer].time);
        }
        
        Signal(audio.main_task, SIGBREAKF_CTRL_D);
      } else
      {   signals = Wait(SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_D);
        if (signals & SIGBREAKF_CTRL_E)
        {   flags |= FLAG_EXIT;
        }   }   }   }
  
  if (link)
  {   u32 sigbit = 1 << link->ahir_Std.io_Message.mn_ReplyPort->mp_SigBit;
    
    signals = Wait(sigbit);
    
    WaitIO((struct IORequest*) link);
  }
  
  if (device == 0)
  {   CloseDevice((struct IORequest*) AHIIO1);
  }
  if (AHIIO2)
  {   DeleteIORequest((struct IORequest*) &AHIIO2->ahir_Std);
  }
  if (AHIIO1)
  {   DeleteIORequest((struct IORequest*) &AHIIO1->ahir_Std);
  }
  if (AHIMP2)
  {   DeleteMsgPort(AHIMP2);
  }
  if (AHIMP1)
  {   DeleteMsgPort(AHIMP1);
  }
  if (timer)
  {   timer_exit(timer);
  }
  
  /* make sure we don't race */
  Forbid();
  
  /* send error signal in case we failed */
  Signal(audio.main_task, SIGBREAKF_CTRL_E);
}

s32 ahi_open(s32 frequency, u32 mode, s32 fragsize, s32 frags)
{   struct TagItem ti[] = {{NP_Entry, (ULONG)ahi_task},
  {NP_Name, (ULONG)ahi_task_name},
  {NP_StackSize, 4 * 65536}, /* 64KB should be enough */
  {NP_Priority, 10},
  {TAG_DONE, 0}};
  s32            i;
  u32            signals;
  
  /* reset structure */
  memset(&audio, 0, sizeof(audio_t));
  
  /* set defaults */
  audio.volume   = 0x10000; // Volume:  0dB
  audio.position = 0x08000; // Panning: center
  
  audio.timer = timer_init();
  if (!audio.timer)
  {   goto fail;
  }
  
  /* samples to bytes */
  audio.samples_to_bytes = 1;
  if (mode & AUDIO_MODE_16BIT)
  {   audio.samples_to_bytes *= 2;
  }
  if (mode & AUDIO_MODE_STEREO)
  {   audio.samples_to_bytes *= 2;
  }
  audio.samples_to_bytes >>= 1; /* we use shifts */
  
  fragsize <<= audio.samples_to_bytes;
  
  /* store settings */
  audio.frequency = frequency;
  audio.mode = mode;
  audio.fragsize = fragsize;
  audio.frags = frags;
  audio.audio_sync = NULL;
  audio.read_position = -1;
  
  /* allocate buffers */
  audio.audio_buffers = (audio_buffer_t*) AllocVec(audio.frags * sizeof(audio_buffer_t), MEMF_PUBLIC | MEMF_CLEAR);
  if (!audio.audio_buffers)
  {   goto fail;
  }
  for (i=0; i < audio.frags; i++)
  {   audio.audio_buffers[i].buffer = AllocVec(fragsize, MEMF_PUBLIC | MEMF_CLEAR);
    if (!audio.audio_buffers[i].buffer)
    {   goto fail;
    }
    audio.audio_buffers[i].size = 0;
  }
  
  /* get task pointer */
  audio.main_task = FindTask(NULL);
  
  /* remove signals */
  SetSignal(0, SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E);
  
  /* allocate semaphore */
  audio.semaphore = (struct SignalSemaphore*) AllocVec(sizeof(struct SignalSemaphore), MEMF_PUBLIC | MEMF_CLEAR);
  if (!audio.semaphore)
  {   goto fail;
  }
  memset(audio.semaphore, 0, sizeof(struct SignalSemaphore));
  InitSemaphore(audio.semaphore);
  
  /* make sure we don't race */
  Forbid();
  audio.audio_process = CreateNewProc(ti);
  Permit();
  
  audio.audio_task = (struct Task*) audio.audio_process;
  if (!audio.audio_task)
  {   goto fail;
  }
  
  signals = Wait(SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E);
  if (signals & SIGBREAKF_CTRL_E)
  {   audio.audio_task = NULL;
    goto fail;
  }
  
  return 0;
  
fail:
  ahi_close(); /* free any allocated resources */
  
  return -1;
}

void ahi_set_volume(s32 volume)
{   audio.volume = volume;
}
void ahi_set_position(s32 position)
{   audio.position = position;
}

static s32 ahi_in_buffer(void)
{   struct timeval dt, ct, st;
  s32            write_position, read_position, in_buffer;
  
  if (!audio.audio_task)
  {   return 0; /* not open */
  }
  
  ObtainSemaphore(audio.semaphore);
  
  timer_gettime(audio.timer, &ct);
  
  write_position = audio.write_position;
  
  if (audio.read_position >= 0)
  {   s64 diff;
    
    st = audio.tv;
    dt = ct;
    
    timer_subtime(audio.timer, &dt, &st);
    
    diff = (dt.tv_secs * 1000000) + dt.tv_micro; /* us */
    diff = (diff * audio.frequency) / 1000000;
    diff <<= audio.samples_to_bytes;
    
    if (diff < 0)
    {   diff = 0;
    } elif (diff > audio.fragsize)
    {   diff = audio.fragsize;
    }
    
    read_position = audio.read_position + diff;
  } else
  {   read_position = 0; /* we haven't started playing anything yet */
  }
  
  ReleaseSemaphore(audio.semaphore);
  
  /* in buffer */
  in_buffer = (write_position - read_position);
  if (in_buffer < 0)
  {   in_buffer += (audio.frags * audio.fragsize);
  }
  
  return in_buffer;
}

void ahi_play_samples(s8* data, s32 size, s64 time, s32 wait)
{   s32 used, copy, max;
  
  if (!audio.audio_task)
  {   return; /* not open */
  }
  
  if (wait == DOWAIT)
  {   /* Here we should wait until the buffer is half empty */
    s32 in_buffer, total_buffer;
    s64 diff;
    
    in_buffer = ahi_in_buffer();
    total_buffer = (audio.fragsize * audio.frags);
    diff = in_buffer - (total_buffer >> 1);
    if (diff > 0)
    {   diff >>= audio.samples_to_bytes;
      diff = (diff * 1000000) / audio.frequency; /* us to wait */
      timer_usleep(audio.timer, diff);
    }   }
  
  if (time != NOTIME)
  {   audio.current_time = time;
  }
  
  time = audio.current_time;
  
  size <<= audio.samples_to_bytes;
  
  audio.play = 1; // start playing
  
  while (size > 0)
  {   for (;;)
  {   ObtainSemaphore(audio.semaphore);
    used = audio.audio_buffers[audio.write_buffer].used;
    ReleaseSemaphore(audio.semaphore);
    
    if (used)
    {   Wait(SIGBREAKF_CTRL_D);
    } else
    {   break;
    }   }
    
    ObtainSemaphore(audio.semaphore);
    
    if (audio.audio_buffers[audio.write_buffer].size == audio.fragsize)
    {   audio.audio_buffers[audio.write_buffer].size = 0;
    }
    
    if (audio.audio_buffers[audio.write_buffer].size == 0)
    {   audio.audio_buffers[audio.write_buffer].time = time;
    }
    
    max = audio.fragsize - audio.audio_buffers[audio.write_buffer].size;
    
    copy = size;
    if (copy > max)
    {   copy = max;
    }
    
    ULONG arg1 = (ULONG) audio.audio_buffers[audio.write_buffer].buffer;
    ULONG arg2 = (ULONG) audio.audio_buffers[audio.write_buffer].size;
    memcpy((void*) (arg1 + arg2), data, copy);
    
    // memcpy(audio.audio_buffers[audio.write_buffer].buffer + audio.audio_buffers[audio.write_buffer].size, data, copy);
    audio.audio_buffers[audio.write_buffer].size += copy;
    data += copy;
    size -= copy;
    
    time += ((copy >> audio.samples_to_bytes) * TIMEBASE) / audio.frequency;
    
    audio.write_position = (audio.write_buffer * audio.fragsize) + audio.audio_buffers[audio.write_buffer].size;
    
    if (audio.audio_buffers[audio.write_buffer].size == audio.fragsize)
    {   audio.audio_buffers[audio.write_buffer].used = 1;
      audio.write_buffer++;
      if (audio.write_buffer >= audio.frags)
      {   audio.write_buffer = 0;
      }   }
    ReleaseSemaphore(audio.semaphore);
    
    Signal(audio.audio_task, SIGBREAKF_CTRL_D); // FIXME: only signal when successful write
  }
  
  audio.current_time = time;
}

s32 ahi_samples_to_bytes(s32 samples)
{   return samples <<= audio.samples_to_bytes;
}
s32 ahi_bytes_to_samples(s32 bytes)
{   return bytes >>= audio.samples_to_bytes;
}
s32 ahi_samples_buffered(void)
{   return ahi_in_buffer() >> audio.samples_to_bytes;
}

s32 ahi_samples_free(void)
{   s32 in_buffer, total_buffer;
  
  in_buffer = ahi_in_buffer();
  total_buffer = (audio.fragsize * audio.frags);
  total_buffer -= in_buffer;
  
  return total_buffer >> audio.samples_to_bytes;
}

void ahi_pause(void)
{   if (audio.audio_task)
{   audio.play = 0; /* stop playing */
}   }

void ahi_close(void)
{   s32 i;
  u32 signals;
  
  if (audio.audio_task)
  {   /* stop server */
    Signal(audio.audio_task, SIGBREAKF_CTRL_E);
    
    /* wait for signal */
    do
    {   signals = Wait(SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E);
    } while (!(signals & SIGBREAKF_CTRL_E));
    
    audio.audio_task = NULL;
  }
  
  if (audio.semaphore)
  {   FreeVec(audio.semaphore);
  }
  
  if (audio.audio_buffers)
  {   for (i = 0; i < audio.frags; i++)
  {   FreeVec(audio.audio_buffers[i].buffer);
  }
    FreeVec(audio.audio_buffers);
  }
  
  if (audio.timer)
  {   timer_exit(audio.timer);
  }
  
  /* reset structure */
  memset(&audio, 0, sizeof(audio_t));
}

struct timer_s* timer_init(void)
{   struct timer_s* timer = (struct timer_s*) AllocVec(sizeof(struct timer_s), MEMF_PUBLIC | MEMF_CLEAR);
  
  if (timer == NULL)
  {   return NULL;
  }
  
  if ((timer->TimerMP = CreateMsgPort()))
  {   if ((timer->TimerIO = (struct timerequest*) CreateIORequest(timer->TimerMP, sizeof(struct timerequest))))
  {   if (OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest*) timer->TimerIO, 0) == 0)
  {   timer->TimerBase = timer->TimerIO->tr_node.io_Device;
    return timer;
  }   }   }
  
  timer_exit(timer);
  
  return NULL;
}

void timer_exit(struct timer_s* timer)
{   if (timer)
{   if (timer->TimerBase)
{   CloseDevice((struct IORequest*) timer->TimerIO);
}
  if (timer->TimerIO)
  {   DeleteIORequest((struct IORequest*) timer->TimerIO);
  }
  if (timer->TimerMP)
  {   DeletePort(timer->TimerMP);
  }
  FreeVec(timer);
}   }

void timer_gettime(struct timer_s* timer, struct timeval* tv)
{   if (timer)
{   GetSysTime((TimeVal*) tv);
}   }
void timer_subtime(struct timer_s* timer, struct timeval* dt, struct timeval* st)
{   if (timer)
{   SubTime((TimeVal*) dt, (TimeVal*) st);
}   }

void timer_usleep(struct timer_s* timer, int us)
{   if (timer)
{   /* setup */
  timer->TimerIO->tr_node.io_Command = TR_ADDREQUEST;
  timer->TimerIO->tr_time.tv_secs    = us / 1000000;
  timer->TimerIO->tr_time.tv_micro   = us % 1000000;
  
  /* send request */
  SetSignal(0, (1 << timer->TimerMP->mp_SigBit));
  SendIO((struct IORequest*) timer->TimerIO);
  Wait((1 << timer->TimerMP->mp_SigBit));
  WaitIO((struct IORequest*) timer->TimerIO);
}   }


