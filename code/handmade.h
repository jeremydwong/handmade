#if !defined(HANDMADE_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */
/* Note(jer):
HANDMADE_INTERNAL:
  0 - BUILD FOR PUBLIC
  1 - BUILD FOR DEV
  
HANDMADE_SLOW:
  0 - NOT SLOW CODE ALLOWED!
  1 - SLOW CODE WELCOME!
*/
#define HANDMADE_INTERNAL 1
   #if HANDMADE_SLOW
   #define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
    #define Assert(Expression)
#endif
//NOTE(jer): should these always be using 64bit? 
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)
#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))
/*
  TODO(casey): Services that the platform layer provides to the game
*/
#if HANDMADE_INTERNAL
internal void *DEBUGPlatformReadEntireFile(char *Filename);
internal void DEBUGPlatformFreeFileMemory(void *Memory);
internal bool32 DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory);
#endif
// todo: swap new old macros

/*
  NOTE(casey): Services that the game provides to the platform layer.
  (this may expand in the future - sound on separate thread, etc.)
*/

// FOUR THINGS - timing, controller/keyboard input, bitmap buffer to use, sound buffer to use

// TODO(casey): In the future, rendering _specifically_ will become a three-tiered abstraction!!!
struct game_offscreen_buffer
{
    // NOTE(casey): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct game_sound_output_buffer
{
    int16 SamplesPerSecond;
    int16 SampleCount;
    int16 *Samples;
    
};

struct game_button_state
{
  int HalfTransitionCount;
  bool32 EndedDown;
};

struct game_controller_input
{
  real32 StartX;
  real32 StartY;
  
  real32 MinX;
  real32 MinY;
  
  real32 MaxX;
  real32 MaxY;

  real32 EndX;
  real32 EndY;
  
  bool32 IsAnalog;

  union 
  {
    game_button_state Buttons[6]; /* data */
    struct 
    { 
      game_button_state Up;
      game_button_state Down;
      game_button_state Left;
      game_button_state Right;
      game_button_state RightShoulder;
      game_button_state LeftShoulder;
    };
  };

};

struct game_input
{
  //todo(jer): insert clock values here.
  game_controller_input Controllers[4];
};

struct game_memory
{
  bool32 IsInitialized;
  uint64 PermanentStorageSize;
  void *PermanentStorage; // NOTE(jer): REQUIRED to be cleared to zero at start. 
  uint64 TransientStorageSize;
  void *TransientStorage; 
};

internal void 
GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer);

//
//
//
struct game_state
{
  int ToneHz;
  int GreenOffset;
  int BlueOffset;
};

#define HANDMADE_H
#endif
