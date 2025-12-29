/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#include <windows.h>
#include <stdint.h>

#define internal static 
#define local_persist static 
#define global_variable static

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

// TODO(casey): This is a global for now.
// in general, do not want to have globals. hard to know where and when they are being accessed
// okay, so you can bump the global  and see what functions generate compile errors...but a cleaner way is grouping/bundling into something smart
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable bool GlobalRunning;

win32_window_dimension Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = (int)(ClientRect.right - ClientRect.left);
    Result.Height = (int)(ClientRect.bottom - ClientRect.top);
    return(Result);
}
internal void RenderWeirdGradient(win32_offscreen_buffer Buffer, int Xoffset, int Yoffset){
    int Pitch = Buffer.Width*4;
    uint8 *Row = (uint8 *)Buffer.Memory; 

    //let's see what the optimizer does (casey)
    
    int Width = Buffer.Width;
    int Height = Buffer.Height;
    for(int Y = 0;
        Y < Buffer.Height;
        ++Y)
        {
            uint32 *Pixel = (uint32 *)Row;
            for (int X = 0;
            X < Buffer.Width;
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
            Row += Buffer.Pitch;
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
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
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

LRESULT CALLBACK
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
{
    WNDCLASS WindowClass = {};
    // overflow the stack!
    //uint8 BigOldBlockOfMemory[100*1024*1024] = {};
    // BigOldBlockOfMemory[99*1024*1024] = 123;
    // for (int i = 0; i < 100 * 1024 * 1024; i += 4096) {
    //     BigOldBlockOfMemory[i] = 1;  // touch each page
    // }
    //Overflow(0);
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
            GlobalRunning = true;
            while(GlobalRunning)
            {
                MSG Message;
               
                while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if(Message.wParam == WM_QUIT){
                        GlobalRunning = false;
                    }
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                
                } 
                RECT ClientRect;
                GetClientRect(Window, &ClientRect);
                int WindowWidth = ClientRect.right - ClientRect.left;
                int WindowHeight = ClientRect.bottom - ClientRect.top;
                Win32ResizeDIBSection(&GlobalBackbuffer, WindowWidth, WindowHeight);
                RenderWeirdGradient(GlobalBackbuffer, XOffset, YOffset);
                Win32DisplayBufferInWindow(DeviceContext, WindowWidth, WindowHeight, &GlobalBackbuffer,0, 0, WindowWidth, WindowHeight);

                ++XOffset;
                YOffset +=2;
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
