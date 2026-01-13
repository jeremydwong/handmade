#notes
### Gotchas (vscode, etc)
"the input line is too long." -> this is a vscode death somehow. not sure why. 

### 2025-01-08 Day 9 Variable pitch sound buffer.
story:
how many games could we try to make in a few days that play with the idea of 100000 guys at once. 100000 sprites. 


### 2025-01-04 Day 8 Testing the sound buffer. 
Ideas:
circular sound-buffer: easy to keep track of true spot with mod %, giving you remainder when you wrap
had to be fairly intelligent about how to keep track of the two stereo 16bit channels, then each time-sample is a DWORD. 

#### Pseudocode: Two-Buffer Setup, Lock, Write, Play

```
1. Create primary buffer (system buffer)
    - CreateSoundBuffer(primaryBufferDesc) -> primaryBuffer

2. Create secondary buffer (our working buffer)
    - Set format: sample rate, channels, bit depth
    - Set size: typically 2 seconds of audio data
    - CreateSoundBuffer(secondaryBufferDesc) -> secondaryBuffer

3. Set format on primary buffer
    - primaryBuffer->SetFormat(waveFormatEx)

4. Lock secondary buffer for writing
    - secondaryBuffer->Lock(offset, bytes, audioPtr1, bytes1, audioPtr2, bytes2, flags)
    - Note: circular buffer may wrap, giving two pointers

5. Write audio data
    - memcpy(audioPtr1, samples, bytes1)
    - if (bytes2 > 0) memcpy(audioPtr2, samples + bytes1, bytes2)

6. Unlock buffer
    - secondaryBuffer->Unlock(audioPtr1, bytes1, audioPtr2, bytes2)

7. Play
    - secondaryBuffer->Play(0, 0, DSBPLAY_LOOPING)

8. Continuously loop: lock -> write new frame -> unlock
```

### 2025-12-31 Day 7 Creating the sound buffer
WIn32 Directsound
- you actually create two buffers, which is kind of stupid
- you pass references to a buffer around so that you can write to the buffer

#### actual audio stuff: 
why do we have 2 seconds?
we have an audiobuffer of some amount that is actually a circular buffer. so you write to a buffer, and then the buffer will loop read (within directsound). 

maybe in general we want to write one or two frames ? 

when you here a stutter in sound, that is the narrow buffer problem. So one approach to avoiding that stutter is, if you have even a hammering of the computer's performance, you'll still be hearing some sensical sound.   

Crazy bug where i was doing a 
if (Message.WParam == WM_QUIT)
when i should have been doing 
(Message.message == WM_QUIT)

crazily enough, WParam was alt, which happened to be the same as WM_QUIT. any other number and i wouldn't have caught the error. 
### 2025-12-29 Day 6 Reading controller state

getting controller state. 
all you do is use XInput, you get a structure, and you can loop. 

#### Linking. the concept of loading windows functions yourself. 
platform requirements : linking XInput.lib . this is somewhat a problem! we do not want to link directly to xinputlib maybe, because there are a lot of weird platform requirements. 
- so can we get away with not launching the game if you don't have an external symbol problem. 

Fix is: loading windows functions ourselves, and using a stub function if we do not have an xinput plugged in. 

to do this involves a few steps that handle the cosmetics of using still the same command (XInputGetState / XInputSetState). 

1. first we do a define statement that sets up a findreplace to create function signatures identical to our true XInputGetState calls (which typically takes in 2 args, a counter and a pointer) and SetState (which takes in 2, a counter and vibration). So we're left with find/replaces for XINPUTGETSTATE(name) and SET()

2. then we do a typedef where we use of the find/replace and return 0 (a stub). so we call it like this: XINPUTRGETSTATE(GETSTUB) {return 0};

3. now that we have those stubs, we assign them to a global variable. 
4. finally we do another define statement that will replace any time we use the true word with our fake word.

### 2025-12-23 Day 5

watch chandler carruth lectures for optimizer compiling. 

"aliasing" -> a cpu performance problem that occurs when you've had coincidences with the addresses in memory. 
"pointer aliasing" -> a language problem. 


#### win32 windows management: very handy to
WNDCLASS WindowClass -> 

In Windows, everything running is a 'Process'. 
Windows will handle messages to that window that get caught by the OS. key-presses, etc. 

We need to keep track of which process is associated with that window. 
We also need to associate a function pointer that gets called when fielding a message.

So, we define an lpfnWinProc 'Win32MainWindowCallback' that will handle all of these messages. hence this
function handling 'close', 'destroy','paint' etc. 

we have to give it a class name, which is a bit useless. 
Interesting to know that we don't actually need to keep around the WindowClass variable. in our case we never leave this function so it never goes out of scope, but Windows actually snaps a copy. 

### 2025-12-19 Day 3

think about controlling data in waves. if you've ever had a program that is slow to close it is probably that you did the RAII. do not think about managing things individually, do things in waves. 

&WindowClass 0000001f3c4ffaf0
later...
memory size 0000001f3c4ff124