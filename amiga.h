typedef short          BOOL; // from <exec/types.h>
typedef unsigned long  ULONG;
typedef unsigned short UWORD;
typedef unsigned char  UBYTE;
typedef unsigned char  FLAG;
typedef char*          STRPTR;

#define TRANSIENT              auto   /* automatic variables */
#define MODULE                 static /* external static (file-scope) */
#define PERSIST                static /* internal static (function-scope) */
#define DISCARD                (void) /* discarded return values */
#define elif                   else if
#define acase                  break; case
#define adefault               break; default
#define EXPORT
#define EOS                    0
#define MAX_PATH               512
#ifdef __amigaos4__
typedef char*   STRING;
#else
typedef UBYTE*  STRING;
#define TimeVal timeval
#endif
#ifdef __SASC
#define CHIP __chip
#else
#define CHIP
#endif

/* format */
#define AUDIO_MODE_8BIT   (0x00) /*  8 bit */
#define AUDIO_MODE_16BIT  (0x01) /* 16 bit */
#define AUDIO_MODE_MONO   (0x00) /*   mono */
#define AUDIO_MODE_STEREO (0x02) /* stereo */

/* format helpers */
#define AUDIO_M8S  (AUDIO_MODE_MONO   | AUDIO_MODE_8BIT ) /*  8 bit signed, mono */
#define AUDIO_M16S (AUDIO_MODE_MONO   | AUDIO_MODE_16BIT) /* 16 bit signed, mono */
#define AUDIO_S8S  (AUDIO_MODE_STEREO | AUDIO_MODE_8BIT ) /*  8 bit signed, stereo */
#define AUDIO_S16S (AUDIO_MODE_STEREO | AUDIO_MODE_16BIT) /* 16 bit signed, stereo */

/* time */
#define TIMEBASE (1000000) /* us */
#define NOTIME   ((s64)-1) /* when no timestamp is available */
#define NOWAIT   (0)
#define DOWAIT   (1)

/* 8bit */
typedef   signed char s8;
typedef unsigned char u8;

/* 16bit */
typedef   signed short s16;
typedef unsigned short u16;

/* 32bit */
typedef   signed long s32;
typedef unsigned long u32;
typedef         float f32;

/* 64bit */
typedef   signed long long s64;
typedef unsigned long long u64;
typedef             double f64;

EXPORT void refresh(void);
EXPORT BOOL aslfile(int kind);
EXPORT void flipulong(ULONG* theaddress);
EXPORT void flipuword(UWORD* theaddress);
EXPORT unsigned int afwrite(const void* ptr, unsigned int size, unsigned int n, FILE* f, UBYTE flip);
EXPORT void settitle(void);
EXPORT void check_timer(void);
EXPORT void set_timer(void);
EXPORT s32 ahi_open(s32 frequency, u32 mode, s32 fragsize, s32 frags);
EXPORT void ahi_play_samples(s8* data, s32 samples, s64 time, s32 wait);
EXPORT void ahi_close(void);
EXPORT void generate_crctable(void);
EXPORT void do_wait(void);
EXPORT void rq(STRPTR text);
EXPORT void say(STRPTR text);
EXPORT void check_keyboard_status(void);
EXPORT UBYTE* displaycallback(void);
