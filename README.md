#  

![ScreenShot](https://raw.github.com/kosmonautdnb/Wypa/main/GITHUB.PNG)
# WYPA
_why you polish abstracts_  

A paintprogram, you can use to paint pictures like in Photoshop, Clipstudio paint, Gimp or Krita and alikes, but in Dos/**FreeDOS**.  

It is completely **OpenSource** under the **MIT license**.  

Featuring a large range of graphics formats. Even some 256 color formats, however Wypa is intended to do **32bit true color** editing of pictures. You can use a palette(realtime quantitation) whilst painting and save the pictures to 256 color tgas, whatsoever.

There is an internal **aspect ratio correction**, if you use a 16:9 monitor with Wypas native resolution of 1024x768.

You can have uneven pixel sizes (like 2:1) to support the oldschool communities.


It also supports TTF truetype fonts with its text layers.  

There are adjustment layers for on the fly post processing of the whole image.  

Cube luts (lookup tables) and a wide range of color grading options are also available whilst painting.

You can create tilesets and **tilemaps** with it and export the tilemaps to simple/readable formats. There is also a seamless mode to have the pencil go out on the left and enter at the right in order to paint seamless tiles.

You can edit and play **sprite animations** with it.  

And you can add a mirror tool, to paint two times with the same pen, but mirrored somewhere in your image.

There is a inapp help available if you have questions to the functionality of any part of Wypa.

## Compilation

You need Djgpp or Watcom C++11 or OpenWatcom in order to get your **Dos exe** file.  

With Djgpp: just run **!DJGPP.BAT**  
With WatcomC/OpenWatcom: run **!WATCOM.BAT**  

If you use DJGPP, be aware, that more recent DJGPPs may bring a lot more of performance to Wypa.

The newest **DJGPP** here is around **3 times** faster than WatcomC++11.  

## Memory and performance

Please be aware that Wypa needs a lot of memory, as stated in the inapp documentation and needs a fairly fast PC. It may already run well with an **around 1600Mhz** setup.

## DosBOX and DosBOX-X

Generally it is not suitable to run Wypa in an emulation like DosBOX (or DosBOX-X) since it will be much too slow.

## Portability

Wypa is written in a most simple C++ dialect. It even runs with compilers from the 90s (WatcomC11). It should be no problem to port it to any platform you like. There is a simple demo of porting it already included in main.cpp. Mainly the file operations may need another porting (dos.cpp). If you need a tiny/lightweight paint program for your platform, maybe Wypa is worth a look. Actually it is not only restricted to FreeDOS.

#  
The original WatcomGL repository lives at: https://github.com/kosmonautdnb/Wypa.git
### (c)2026 by Stefan Mader of Puls4r


