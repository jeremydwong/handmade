notes
### 2025-12-31 Day 7

Crazy bug where i was doing a 
if (Message.WParam == WM_QUIT)
when i should have been doing 
(Message.message == WM_QUIT)

crazily enough, WParam was alt, which happened to be the same as WM_QUIT. any other number and i wouldn't have caught the error. 
### 2025-12-29 Day 6

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