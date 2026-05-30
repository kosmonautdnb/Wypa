# Wypa
A tiny and fastcoded swift and DOS based Photoshop like picture edit program. (in development). 

Tiny usage video here:  
https://www.youtube.com/watch?v=420j0mS6s1w&pp  
(you can do tilemaps and isometric tilemaps with Wypa)  

![ScreenShot](https://raw.github.com/kosmonautdnb/Wypa/main/GITHUB.PNG)

## "WYPA" is an easy-to-use graphics program.. if you know how.  

If you want to use a very simple graphics program on DOS/FreeDOS, you should  
check out Grafx2, Deluxe Paint, or Paintbrush. Wypa is more for  
people who already have some experience with Photoshop, Clip Studio Paint,  
Krita, or even GIMP.  

With Wypa, we focused more on making the program itself relatively simple  
rather than prioritizing user interests.  

The goal was actually to create a program that allows you to draw in  
True Color on a 16:9 monitor in FreeDOS.  

If you know how, almost everything is available and manageable, just like  
in the big programs such as Photoshop, etc.  

The first important difference is that the layers in the program mostly retain their  
resolution and would have to be "baked" to combine them. This happens  
automatically when simply merging the layers.  

One of the biggest drawbacks is that the Undo function does not include  
adding or deleting layers. So, if you delete a layer, you cannot restore  
it using Undo. Sorry.  

For simple drawing, like in DeluxePaint, the "Pencil" tool is sufficient.  
Right-click to pick up the color from the image, and left-click to place a  
pixel. If you set the brush to a larger size, you can also "fill" larger  
areas at once.  

The "Fill" tool and the "Line" tool are also almost similar to those  
in DeluxePaint.  

Btw. it is important to always make sure that "Selection" is not enabled, so  
you do not end up drawing outside the selection itself.  

The "Selections" are linked to the currently selected layer, and you will  
need to transfer them via a menu option or create new ones if you switch  
layers.  

You can also press Control while switching layers; this will copy the  
"Selection" as you switch layers.  

Copy and Paste work with Ctrl-C and Ctrl-V; you can also use  
Ctrl-C and Ctrl-V again right away to copy once more.  

It is important that you then merge the pasted layer back with the  
layer below it using the "Merge" menu option.  

If you want to adjust the colors, you can use the "Postprocessing" tool.  
This allows you to change the colors of the entire image or layer.  
However, this is really more for professionals who use Photoshop  
and other programs.  

You can add text to the image, but to do so you will need to get your hands on  
some ".ttf" TrueType fonts. The program only comes with one public domain  
TrueType font. When it comes to fonts, always pay attention to the  
copyright.. you would not believe it, but this is a common issue with fonts.  

Overlays are a key feature in Wypa. You can use them to place sprites and  
lines over the image, for example, to draw precisely along straight lines or  
to create tileable/seamless tiles.  Or you can simply assemble a small scene  
using overlays or sprites to see what the scene looks like while you edit the  
source data.  

The program includes a lot of professional features, and DeluxePaint or  
similar programs might be a better choice if you want to create graphics  
in DOS.  

Wypa is more of a powerful program for designing games or similar  
projects in True Color.  


The key points are:  
- Selections are per layer  
- Layers are baked when they are merged  
- Deleting, merging, or inserting layers cannot be undone  
- It is best to press 'k' or the "!" button before merging layers to align the layer coordinates  


When using the Move tool, you can use the  
'1' to '9' keys on the numeric keypad to move a layer by one pixel.  

Use Alt and the mouse to move the entire image on the screen.  

The arrow keys also move the image on the screen.  
