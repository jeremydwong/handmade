/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#include <windows.h>
#include <stdint.h>
#include <Xinput.h>
#include <dsound.h>

#define internal static         // define file scope functions as internal
#define local_persist static    // static means different things in different contexts. local_persist across calls.
#define global_variable static  // genuine global variable; try to remove as much as possible.

// type definitions for fixed width types; generally easier to read and type; bool32 clarifies the actual true or false operation. 
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct win32_window_dimension
{
    int Width; 
    int Height;
};

struct win32_offscreen_buffer
{ BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

// TODO(jer): This is a global for now.
// in general, do not want to have globals. hard to know where and when they are being accessed
// okay, so you can bump the global  and see what functions generate compile errors...but a cleaner way is grouping/bundling into something smart
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable bool GlobalRunning;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

// NOTE(jer) XInputGetState.
// XInput function types XInputGetState and XInputSetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{ return ERROR_DEVICE_NOT_CONNECTED;} //be careful! 0-> is actually_ERROR_SUCCESS. 
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{ return ERROR_DEVICE_NOT_CONNECTED;}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState  XInputSetState_

//sound
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuiDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void 
Win32LoadXInput(void)
{
    // todo(jer): diagnostic so that when we do error checking, we know which got loaded. 
    HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
    if (!XInputLibrary) {
        HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll"); //we do not have 1.3 in windows>8. so we can't rely on that anymore...
        if (XInputLibrary) {
            void *temp = GetProcAddress(XInputLibrary, "XInputGetState");
            XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
            if(!XInputGetState) {XInputGetState = XInputGetStateStub;}
            XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
            if(!XInputSetState) {XInputSetState = XInputSetStateStub;}
        }
        else
        {
            // TODO(jer): Logging
        }
    
}
}

internal void 
Win32initDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
    // NOTE(jer): load, then get DirectSound object! coopperative

    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    DSBUFFERDESC BufferDescription = {};
    if (DSoundLibrary)
    {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)
            GetProcAddress(DSoundLibrary, "DirectSoundCreate");
        
        LPDIRECTSOUND DirectSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {   
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2; //stereo
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample)/8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;
            if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                BufferDescription.dwSize = sizeof(BufferDescription);

                // Note(jer): then we will create a primary buffer, where you wrote directly to the card...
                // Note(jer): global focus ? 
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                //Primary buffer is really just setting the hardware buffer format. don't think of as buffer. 
                if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
                {   HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat); 
                    if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
                    {
                        // NOTE(jer): We finally set the format! 
                    }
                    else
                    {
                        // TODO(jer): diagnostic
                    }
                }
                else 
                {
                    // TODO(jer): diagnostic
                }
            
            }
            else
            {
                // TODO(jer): diagnostic
            }
            // TODO(jere): GETCURRENTPOSITION2
            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
            HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);

            if(SUCCEEDED(Error))
            {
                OutputDebugStringA("Secondary buffer created successfully.\n");
            }
            else 
            {
                OutputDebugStringA("Failed to create secondary buffer.\n");
                // TODO(jer): diagnostic
            }
            
            // then we will create a secondary buffer which is where we will actually write to
            BufferDescription.dwBufferBytes = BufferSize;
            // then we will start it playing!
        }

    }
    // would rather not have to deal with COM. Unfortunately this is just the best API...
    
}

internal win32_window_dimension Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = (int)(ClientRect.right - ClientRect.left);
    Result.Height = (int)(ClientRect.bottom - ClientRect.top);
    return(Result);
}
internal void RenderWeirdGradient(win32_offscreen_buffer *Buffer, int Xoffset, int Yoffset){
    int Pitch = Buffer->Width*4;
    uint8 *Row = (uint8 *)Buffer->Memory; 

    //let's see what the optimizer does (casey)
    
    int Width = Buffer->Width;
    int Height = Buffer->Height;
    for(int Y = 0;
        Y < Buffer->Height;
        ++Y)
        {
            uint32 *Pixel = (uint32 *)Row;
            for (int X = 0;
            X < Buffer->Width;
            ++X)
            {   /* pixel in memory: 
                00 00 00 00 
                little-endian, so RGB, B is lowest. 
                so 0x XXRRGGBB , looking at it as a vector:
                for a fun day4, set the first two bytes to (uint8)(X/4) and (uint8)(Y/4)
                */
               uint8 blue = (uint8)(X+Xoffset);
               uint8 green = (uint8)(Y+Yoffset);
               uint8 red = 0;
               uint8 Xalpha = 0;
               //*Pixel = (Xalpha << 24) | (red << 16) | (green << 8) | blue;
               *Pixel = (green << 8) | blue;
               
            //    *Pixel = 0;
               
            //    *Pixel = 0;
               Pixel++;
                     
            }
            Row += Buffer->Pitch;
        }

}

//truly this is like create bitmap buffer, rather than a resize.
internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    // TODO(casey): Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails.
if(Buffer->Memory)
{
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE); //0-> just the size we allocated before
    // one way we could catch bugs of pointers to stale memory is to do VirtualProtect
    // to no access here, so that if we try to use it again we will get a page fault.
}

    Buffer->Width = Width;
    Buffer->Height = Height;
    int BytesPerPixel = 4;
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader); //cool. here we are going to pass biSize so that we can do pointer arithmetic under the hood in CreateDIBSection
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height; // define DIB as top-down
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32; //R, G, B, each 8 bits, and a padding byte.
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    // TODO(casey): Based on ssylvan's suggestion, maybe we can just
    // allocate this ourselves?
    // thanks to chris hecker of spy party fame, for clarifying the deal with stretchDIBits and BitBlt!
    // no more DC needed for us. 

    // now set the bitmap memory, it was a pointer.
    int BitmapMemorySize = (Buffer->Width * Buffer->Height)*BytesPerPixel;
    // here we use virtualalloc, a bit more raw, returns pages. so 4096 byte aligned. 12 bits.
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width*BytesPerPixel;
        
}

internal void
Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight,  win32_offscreen_buffer *Buffer, int X, int Y, int Width, int Height)
{
    /* 
     convention for storing something 2D in something that is 1D. 
     a series of 1D slices that need to be reassembled. 
     that thing is called a pitch or a stride. 
     when you move a pointer, typically something like a pitch, the value you would add to the pointer
        to get to the next row is called the pitch
        the stride is the thing you'd typically add to the end is the stride.
    */
    //DIB->DeviceIndependentBitmap
    // TODO: (casey says): aspect ratio correction
    StretchDIBits(DeviceContext,
                  /*X, Y, Width, Height,
                  X, Y, Width, Height,*/
                  0,0, WindowWidth,WindowHeight,
                  0,0, Buffer->Width,Buffer->Height,
                  Buffer->Memory,
                  &Buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);
                  // DIB_RGB_COLORS: we are specifying colors directly
                    // SRCCOPY: raster operation, just copy source directly to dest
                  // historical; bitblt() used to be faster on older windows versions. 
                  // now stretchis probably okay for now;
                  // we may want to use opengl and eventually write to
}

internal LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                        UINT Message,
                        WPARAM WParam,
                        LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {   
        
        case WM_SIZE:
        {
            
        } break;

        case WM_CLOSE:
        {
            // TODO(casey): Handle this with a message to the user?
            GlobalRunning = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_DESTROY:
        {
            // TODO(casey): Handle this as an error - recreate window?
            GlobalRunning = false;
        } break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYUP:
        case WM_KEYDOWN:
        {
            uint32 VKCode = WParam;
            bool WasDown = ((LParam & (1 << 30)) != 0); // 30 is previous state. 
            bool IsDown = ((LParam & (1 << 31)) == 0);  // 31 is current state.
            if(WasDown != IsDown)
            {
                if(VKCode == 'W')
                {
                }
                else if(VKCode == 'A')
                {
                }
                else if(VKCode == 'S')
                {
                }
                else if(VKCode == 'D')
                {
                }
                else if(VKCode == 'Q')
                {
                }
                else if(VKCode == 'E')
                {
                }
                else if(VKCode == VK_UP)
                {
                }
                else if(VKCode == VK_LEFT)
                {
                }
                else if(VKCode == VK_DOWN)
                {
                }
                else if(VKCode == VK_RIGHT)
                {
                }
                else if(VKCode == VK_ESCAPE)
                {
                    OutputDebugStringA("ESCAPE: ");
                    if(IsDown)
                    {
                        OutputDebugStringA("IsDown ");
                    }
                    if(WasDown)
                    {
                        OutputDebugStringA("WasDown");
                    }
                    OutputDebugStringA("\n");
                }
                else if(VKCode == VK_SPACE)
                {
                }
                
            }
            bool32 AltKeyWasDown = (LParam & (1 << 29)); // alt key has separate bit. 0 is not down.
            if((VKCode==VK_F4) && AltKeyWasDown)
            {
                GlobalRunning = false;
            } 
        } break;
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackbuffer, X, Y, Width, Height);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
//            OutputDebugStringA("default\n");
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    
    return(Result);
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{   Win32LoadXInput();

    WNDCLASSA WindowClass = {};
    Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

    // TODO(casey): Check if HREDRAW/VREDRAW/OWNDC still matter
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
//    WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if(RegisterClassA(&WindowClass))
    {
        HWND Window =
            CreateWindowExA(
                0,
                WindowClass.lpszClassName,
                "Handmade Hero",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                Instance,
                0);
        if(Window)
        {
            HDC DeviceContext = GetDC(Window);

            int XOffset = 0;
            int YOffset = 0;
            
            // NOTE(casey): Sound test
            int SamplesPerSecond = 48000;
            int ToneHz = 256;
            int16 ToneVolume = 3000;
            uint32 RunningSampleIndex = 0;
            int SquareWavePeriod = SamplesPerSecond/ToneHz;
            int HalfSquareWavePeriod = SquareWavePeriod/2;
            int BytesPerSample = sizeof(int16)*2;
            int SecondaryBufferSize = SamplesPerSecond*BytesPerSample;

            Win32initDSound(Window, 48000, 48000*sizeof(int16)*2); //stereo
            bool32 SoundIsPlaying = false;

            GlobalRunning = true;

            while(GlobalRunning)
            {
                MSG Message;
               
                while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if(Message.message == WM_QUIT){
                        GlobalRunning = false;
                    }
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                
                } 
                for (DWORD ControllerIndex = 0;
                ControllerIndex < XUSER_MAX_COUNT;
                ++ControllerIndex)
            {   
                XINPUT_STATE ControllerState;
                if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                {
                    // this controller is plugged in!
                    //todo(jer): see if controllerstate.dwpsacketnumbrer increments too rapidly
                    // which would be a sign that we're not polling it fast enough
                    XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
                    bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                    bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                    bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                    bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                    bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                    bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                    bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                    bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                    bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                    bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                    bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);
                    int16 StickX = Pad->sThumbLX;
                    int16 StickY = Pad->sThumbLY;

                    XOffset += StickX >> 12;
                    YOffset += StickY >> 12;
                    if (AButton)
                        { YOffset +=2;}
                    
                }

            }
                
                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                RenderWeirdGradient(&GlobalBackbuffer, XOffset, YOffset);

                 // NOTE(jer): DirectSound output test
                DWORD PlayCursor;
                DWORD WriteCursor;
                if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                {
                    DWORD ByteToLock = RunningSampleIndex*BytesPerSample % SecondaryBufferSize;
                    DWORD BytesToWrite;
                    if(ByteToLock == PlayCursor)
                    {
                        BytesToWrite = SecondaryBufferSize;
                    }
                    else if(ByteToLock > PlayCursor)
                    {
                        BytesToWrite = (SecondaryBufferSize - ByteToLock);
                        BytesToWrite += PlayCursor;
                    }
                    else
                    {
                        BytesToWrite = PlayCursor - ByteToLock;
                    }

                    // TODO(casey): More strenuous test!
                    // TODO(casey): Switch to a sine wave
                    VOID *Region1;
                    DWORD Region1Size;
                    VOID *Region2;
                    DWORD Region2Size;
                    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
                                                             &Region1, &Region1Size,
                                                             &Region2, &Region2Size,
                                                             0)))
                    {
                        // TODO(casey): assert that Region1Size/Region2Size is valid

                        // TODO(casey): Collapse these two loops
                        DWORD Region1SampleCount = Region1Size/BytesPerSample;
                        int16 *SampleOut = (int16 *)Region1;
                        for(DWORD SampleIndex = 0;
                            SampleIndex < Region1SampleCount;
                            ++SampleIndex)
                        {
                            int16 SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? ToneVolume : -ToneVolume;
                            *SampleOut++ = SampleValue;
                            *SampleOut++ = SampleValue;
                        }

                        DWORD Region2SampleCount = Region2Size/BytesPerSample;
                        SampleOut = (int16 *)Region2;
                        for(DWORD SampleIndex = 0;
                            SampleIndex < Region2SampleCount;
                            ++SampleIndex)
                        {
                            int16 SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? ToneVolume : -ToneVolume;
                            *SampleOut++ = SampleValue;
                            *SampleOut++ = SampleValue;
                        }

                        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
                    }
                }

                if(!SoundIsPlaying)
                {
                    GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
                    SoundIsPlaying = true;
                }

                Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackbuffer,0, 0, Dimension.Width, Dimension.Height);


            }
        }
        {
            // TODO(casey): Logging
        }
    }
    else
    {
        // TODO(casey): Logging
    }
    
    return(0);
}
