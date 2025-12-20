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
// TODO(casey): This is a global for now.
global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory; //later we will cast this to more than one type
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;
global_variable int XOffset = 0;
global_variable int YOffset = 0;

internal void RenderWeirdGradient(int Xoffset, int Yoffset){
    int Pitch = BitmapWidth*4;
    uint8 *Row = (uint8 *)BitmapMemory; 

    int Width = BitmapWidth;
    int Height = BitmapHeight;
    for(int Y = 0;
        Y < BitmapHeight;
        ++Y)
        {
            uint8 *Pixel = (uint8 *)Row;
            for (int X = 0;
            X < BitmapWidth;
            ++X)
            {   /* pixel in memory: 
                00 00 00 00 
                little-endian, so RGB, B is lowest. 
                so 0x XXRRGGBB , looking at it as a vector:
                for a fun day4, set the first two bytes to (uint8)(X/4) and (uint8)(Y/4)
                */
               *Pixel = (uint8)(X+Xoffset);
               ++Pixel;
               *Pixel = (uint8)(Y+Yoffset);
               ++Pixel;
               *Pixel = 0;
               ++Pixel;
               *Pixel = 0;
               Pixel++;
                     
            }
            Row += Pitch;
        }

}
internal void
Win32ResizeDIBSection(int Width, int Height)
{
    // TODO(casey): Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails.
if(BitmapMemory)
{
    VirtualFree(BitmapMemory, 0, MEM_RELEASE); //0-> just the size we allocated before
    // one way we could catch bugs of pointers to stale memory is to do VirtualProtect
    // to no access here, so that if we try to use it again we will get a page fault.
}

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader); //cool. here we are going to pass biSize so that we can do pointer arithmetic under the hood in CreateDIBSection
    BitmapInfo.bmiHeader.biWidth = Width;
    BitmapInfo.bmiHeader.biHeight = -Height; // define DIB as top-down
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32; //R, G, B, each 8 bits, and a padding byte.
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    // TODO(casey): Based on ssylvan's suggestion, maybe we can just
    // allocate this ourselves?
    // thanks to chris hecker of spy party fame, for clarifying the deal with stretchDIBits and BitBlt!
    // no more DC needed for us. 

    BitmapWidth = Width;
    BitmapHeight = Height;
    // now set the bitmap memory, it was a pointer.
    int BytesPerPixel = 4;
    int BitmapMemorySize = (BytesPerPixel * BitmapWidth * BitmapHeight);
    // here we use virtualalloc, a bit more raw, returns pages. so 4096 byte aligned. 12 bits.
    BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    RenderWeirdGradient(XOffset,YOffset);
        
}

internal void
Win32UpdateWindow(HDC DeviceContext, RECT *WindowRect,  int X, int Y, int Width, int Height)
{
    /* 
     convention for storing something 2D in something that is 1D. 
     a series of 1D slices that need to be reassembled. 
     that thing is called a pitch or a stride. 
     when you move a pointer, typically something like a pitch, the value you would add to the pointer
        to get to the next row is called the pitch
        the stride is the thing you'd typically add to the end is the stride.
    */
    int WindowWidth = WindowRect->right - WindowRect->left;
    int WindowHeight = WindowRect->bottom - WindowRect->top;
    //DIB->DeviceIndependentBitmap
    StretchDIBits(DeviceContext,
                  /*X, Y, Width, Height,
                  X, Y, Width, Height,*/
                  0,0, BitmapWidth,BitmapHeight,
                  0,0, WindowWidth, WindowHeight,
                  BitmapMemory,
                  &BitmapInfo,
                  DIB_RGB_COLORS, SRCCOPY);
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
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            int Width = ClientRect.right - ClientRect.left;
            int Height = ClientRect.bottom - ClientRect.top;
            Win32ResizeDIBSection(Width, Height);
        } break;

        case WM_CLOSE:
        {
            // TODO(casey): Handle this with a message to the user?
            Running = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_DESTROY:
        {
            // TODO(casey): Handle this as an error - recreate window?
            Running = false;
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

            Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
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
    
    // TODO(casey): Check if HREDRAW/VREDRAW/OWNDC still matter
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
            Running = true;
            while(Running)
            {
                MSG Message;
               
                while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if(Message.wParam == WM_QUIT){
                        Running = false;
                    }
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                
                }
                RenderWeirdGradient(XOffset, YOffset);
                XOffset++;
                YOffset++;
                if(XOffset > BitmapWidth){
                    XOffset = 0;
                    
                    if(YOffset > BitmapHeight){
                        YOffset = 0;
                    }
                }
                HDC DeviceContext = GetDC(Window);
                RECT ClientRect;
                GetClientRect(Window, &ClientRect);
                int WindowWidth = ClientRect.right - ClientRect.left;
                int WindowHeight = ClientRect.bottom - ClientRect.top;
                Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);
                ReleaseDC(Window, DeviceContext);
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
