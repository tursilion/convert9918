Convert9918 - A simple image converter for the TMS9918A
-------------------------------------------------------

Really basic - launch it, then either click 'open' or drag and
drop your image file onto the window. It will be converted for
the 9918A's Graphics II mode (Bitmap). There are no options for
other modes.

The current palette is based on the Classic99 palette.

There are a few controls available once the image is loaded:

-Nudge Right & Left - because Bitmap mode has color 
limitations for every 8 horizontal pixels, you can sometimes 
get a better result by shifting a few pixels. It never makes
sense to shift more than 7 pixels, so the program will stop
there. Usually after 4 pixels you are better off trying the
other direction. Loading a new image clears the nudge count.

-Nudge Up & Down - these shift the image up and down one pixel
at a time. Hold Shift for two pixels at a time, hold control for
4 at a time, and hold shift and control together for 8 at a time.
Up and Down refers to the virtual 'camera' direction and only
applies if the image is larger than the display due to one of 
the 'Fill Mode's in the options being set. Scrolling farther
than is possible will simply make it stop moving - the counter
will still change. You can see this in the output text window.
Loading a new image clears the nudge count.

-Stretch Hist (Hist) - this stretches the histogram of the 
image to improve contrast and enhance colors. It can also cause 
a pretty severe color shift, but in some cases it helps the 
detail of the image.

-Use Perceptual Color matching (Perc) - this changes the color
matching system to favour a model of human color visibility,
giving green the most importance, red second, and blue third.
This can sometimes improve an image and provide more pleasing 
results, but it tends to introduce a blue shift in dark areas.
When off, the default matching uses the YCrCb color space.

-Max Color Shift - expressed as a percentage from 0-100%, indicates
how far across the color space a color is allowed to shift towards
a TI color before dithering takes place. The default is 1%, values
of 15% or greater tend to result in a full remapping in most images.
1-3% is adequate for most images, but it may be useful in some
cartoon-like images for forcing more of a color shift and less dithering
in large solid areas (without completely throwing out dithering).

-Dithering - This system uses dithering to improve the apparent 
color accuracy of the image. You can change the error distribution 
in the dialog. Each value indicates the ratio, out of 16, of the 
error that goes to that pixel. In addition, four buttons set
recommended defaults:

Atkinson - sets a modified Atkinson dither
Floyd-Stein - sets Floyd-Steinberg dithering
Pattern - sets a halt-tone-like dither (every other pixel)
Diag    - similar to Atkinson, but a modified pattern that
          tends to produce diagonal hash-line lines
Order1 - ignores the pattern and uses an aligned/ordered dither. 
         The slider underneath can be used to brighten/darken
Order2 - like order1, but allows for error distribution as well.
         Values should be kept equal and small for best results.
No Dither - disables dithering

Note that if the total of all values entered is less than 16,
some of the error will be lost. If the value is more than 16,
errors will accumulate and may induce artifacts.

The 'error mode' has two values that control how the software
deals with error correction distributed from multiple pixels.
In most cases, a pixel will receive error distribution data from
three other pixels. 'Average' mode (the default) will use the
average error when calculating a new pixel. Accumulate is the
old mode, and lets the error total up when calculating a new
pixel. Average is arguably more correct but sometimes Accumulate
gives better results (although it is usually noisier).

You can set the ratios used for the Perceptual filter. Set a value
from 0-100 percent for each of the red, green and blue channels.
Ideally they should total 100, otherwise the image will definately
change. A lower value indicates that the quality of the match is
less important (allowing the color to substitute for others). A
higher value means the quality of the match is more important -
in non-perceptual mode all three have a setting of 100. The default
settings of 30% for red, 59% for green and 11% for blue represent
the scale traditionally used for converting RGB to luminence.

When not using Perceptual mode, you can set a Luma Emphasis, this controls
how much more important brightness changes are than color changes.
Lower values tend to reduce contrast, higher values tend to cause color
shifts.

The 'Gamma' setting applies a Gamma correction to the original image.
The value entered here is treated as a percentage, so 100 is equal to
a gamma of 1.0 (no correction). Higher values will brighten dark areas,
lower values will increase shadows.

This dialog also allows you to choose between various
resizing filters, the default is bilinear.

It also has a dropdown for a 'fill mode'. This affects images
which are not in a 3:2 ratio. In Full mode (the default), the
entire image is scaled to fit. Otherwise, the image is scaled
so that the smaller axis fits the screen, and the rest is
cropped off based on your setting, which can be top/left,
middle, or bottom/right (the setting is what part is kept).

Next is a box to adjust how much flicker is 
acceptable for half-multicolor. Increasing the value allows
more mixes of colors, but the colors may flicker more strongly
when displayed. This slider adjusts the percentage of luminence
difference that is allowed.

'Per-Scanline Static Color Count' is used in the F18A palette
per scanline mode to set how many colors in the palette are fixed
and do not change per line. This can help reduce scanline noise.

'Static color Selection Mode' is used in both F18A palette modes
for selecting the colors that don't change (in the case of the
non-per-scanline mode, that's the whole image.) Median Cut uses
an approximation algorithm to try and find colors that will blend
nicely, while Popularity chooses the most common colors in the
image.

-Open - opens a new file. Many image types including TI Arist
are supported. Half-Multicolor is not at this time.

-Save Pic - saves the color and pattern data. The filename
you enter has .TIAP and .TIAC (and .TIAM if appropriate) 
appended to it, respectively.

Six file types are available:

TIFILES - this is useful for transfer to a real TI, or for
use with Classic99. This header is 128 bytes.

V9T9 - this puts a V9T9 header on the file. This file format
is largely obsolete and I don't recommend using it unless
you know why you need it. This header is 128 bytes. These
files will only use the first 6 characters of the filename
and will store _P and _C (and _M) as the extension.

RAW - no header is attached. Just a raw binary dump of the
color table and pattern table.

RLE - no header is attached. The files are RLE encoded as such:
Count Byte: 
	- if high bit is set, remaining 7 bits indicate to copy
	the next byte that many times
	- if high bit is clear, remaining 7 bits indicate how
	many data bytes (non-repeated) follow
RLE compression will probably not be useful on complex images.
Palette files for the F18A are NOT compressed.

XB Program - saves as an Extended BASIC program with a TIFILES header.
This program displays the image and waits for the user to press
enter. The image and assembly are all saved with the program.
Only Bitmap 9918A modes are supported.

XB RLE Program - also saves as an Extended BASIC program with a
TIFILES header, but RLE compresses the image. Only Bitmap 9918A 
modes are supported.

MSX .SC2 - this is basically a dump of VRAM starting at >0000
and continuing to >3800, with the pattern table at >0000, the
color table at >2000, the SIT at >1800 and the sprite attribute
table at >1B00. According to http://msx.jannone.org/conv/, in 
MSX Basic you can view the image with this short program:
	10 SCREEN 2
	20 BLOAD "FILENAME.SC2",S
	30 GOTO 30
Only Bitmap 9918A modes are supported.

ColecoVision cartridge - This is just a simple .ROM file for 
ColecoVision that displays the picture. You can load it in 
an emulator or burn it to EPROM. Only Bitmap 9918A modes are 
supported.

PNG file (PC) - saves the converted image as a paletted PNG
file for editting on the PC. If you reload the editted file
(without changing the palette) and no new pixel clashes are
introduced, it will convert exactly back into TMS9918A mode.

-A dropdown now select the conversion mode. There are eight modes:

	Bitmap 9918A - the default bitmap (M3) graphics mode. This
	writes a 6k Pattern table (.TIAP) and a 6k Color table (.TIAC).
	You should manually initialize the SIT with three sets of
	>00 to >FF bytes.

	Greyscale Bitmap 9918A - this is displayed exactly the same
	way as regular bitmap, but will map only to black, white and
	grey, no color will be added to the image. If you enable
	PERC and adjust the RGB values, you can tune the levels of the
	conversion to greyscale.

	B&W Bitmap 9918A - this is a bitmap mode screen where all colors
	are expected to be initialized to 0x1F (black on white). The
	color table will not be written to disk. The image is converted
	to greyscale before being dithered to black and white. If you 
	enable PERC and adjust the RGB values, you can tune the levels of the
	conversion.

	Multicolor 9918 - the 64x48 blocky Mode (M2). This writes a
	single 1536 byte pattern table (.TIAP). You should initialize
	your screen image table in six sets of 4 rows, The first set
	repeating the characters >00 to >1F, the next >20 to >3F, etc.

	Dual-Multicolor (flicker) 9918 - this writes two 64x48 blocky
	mode (M2) pattern tables, with the intention that you shall
	flicker between them on the vertical blank. The setup is the
	same as for regular multicolor, but you will get a TIAP and a
	TIAC file. Load both into VRAM at appropriate offsets and
	alternate between them using VDP Register 4 on the vertical
	blank interrupt, and you should get a decent approximation
	of the enhanced colors.

	Half-Multicolor (flicker) 9918A - this is an enhanced concept
	that flickers bitmap mode with multicolor mode to get the
	illusion of improved colors with higher resolution. See the
	description below for setup details. This writes a 6144 bytes
	Pattern table (TIAP), a 6144 byte Color table (TIAC), and a
	2048 byte Multicolor mode Pattern table (TIAM).

	Paletted Bitmap F18A - This defines a 15-color palette that
	matches the image, and reduces it to that palette. TIAP and
	TIAC are setup as per normal bitmap, but a 32-byte TIAM is
	also written containing the palette data. 16 palette entries
	are listed in this file at 16-bits each, big endian. The palette
	data is 12 actual bits, formatted as 0000RRRRGGGGBBBB. This
	file is NEVER compressed, even if compression is selected
	for the other files. You should display the image as a normal
	bitmap but also load the F18A palette.

	Scanline Palette Bitmap F18A - This is similar to the Paletted
	Bitmap F18A, but the palette file is now 6144 bytes and
	contains a unique 32-byte palette for each of the 192 scanlines.
	A GPU program must be loaded on the F18A to change the palette
	every scanline. This mode takes 16.5k and so some GPU code is
	also needed to load some palette data into the extended VRAM.
	A recommended memory layout is suggested below. 

	Note that the current color selection algorithm for Scanline
	palettes is fairly naive, looking only at the current line.
	The 'number of static colors' can be used to help offset the
	problem of scanlines with very different colors from those
	around them (horizontal streaks appearing). This algorithm
	will be improved over time.

-Reload - most configuration and setting changes do not re-render
the image, because of how long this takes. Press Reload to reload and
re-render the image. If you try to save without pressing Reload,
the system will remind you.

-Show Palette - displays the per-scanline palette to the right of the
image. You must select Reload to display it.

If you are using an emulator, Classic99 can read the TIFILES 
or V9T9 files directly into TI Artist or another paint program. 
(TI Artist under Classic99 can load filenames much longer
than 10 characters!)

A Special mode also exists where you can randomly
browse an entire folder (this was me being silly). 
Double click anywhere on the dialog where there is no
button, and 'Open' will become 'Next'. Click it, and
select a file in a folder you would like to browse.
All images in that folder (and all subfolders) will
be available in random order each time you click 'Next'.
You can process and save any you like. To exit this
mode you must close the program.

Finally, there is a simple command line interface.
Passing one filename will cause that image to be loaded and
displayed in the GUI automatically.
Passing a second makes the result automatically be saved to
that filename without displaying the GUI. Three files will be
saved, TIFILES P and C files, and a BMP showing the result.

Source code is not available as this program uses
the ImgSource Library by Smaller Animals Software.

Half-Multicolor Mode VDP Layout
-------------------------------

This mode works by flickering Multicolor and Bitmap on alternate frames,
and it uses most of the 16k of video RAM. Here is the intended layout:

Register setup (if not in both columns, the value doesn't change)

Register	Bitmap		Multicolor
0 - Mode0	02				00
1 - Mode1	E0				E8			(sprite size and interrupts may vary these values)
2 - SIT		0E
3 - CT		FF
4 - PT		03
5 - SDT		76
6 - SPT		07
7 - Back	01								(or whatever you like - background screen color)

So during the vertical blank, just update registers 0 and 1 (four VDP byte writes).

You must manually set up the Screen Image Table. Normally, for bitmap you would
write the values 0-255 three times, and for multicolor you would write sets of
four rows. Because these very different modes are sharing the same table, though,
it's slightly different:

-First write the characters 0-255 as normal
-In the second group, write the characters 0-255 but start at 32, not 0 (let it wrap around)
-In the third group, write the characters 0-255 but start at 64, not 0 (let it wrap around)

Then load the tables as per the map below. Once everything is in memory, just alternate
setting the values in register 0 and register 1 during vertical blank.

MEMORY MAP:

Bitmap pattern:		0000 - 17FF		1800 bytes	(load the TIAP here)
Multi pattern: 		1800 - 1FFF 	0800 bytes	(load the TIAM here)
Bitmap Color:			2000 - 37FF		1800 bytes	(load the TIAC here)
Shared Screen:		3800 - 3AFF		0300 bytes
Sprites Desc:			3B00 - 3B7F		0080 bytes
Sprite Pattern:		3800 - 3FFF		0800 (only room for 0480, which is 144 chars)

Sprite patterns can then only use from 3B80 - 3FFF, which is chars 112-255.

If you want disk to work, use CALL FILES(1). This sets top of VRAM to >3BE3,
which means sprite patterns can only use from >3B80 to>3BE3, or 64 bytes. This is
just 8 chars, from 112-119. Alternately you can back up and restore the VRAM
when needed, just turn off the sprites during disk access.

You don't need to read the rest of this unless you want more technical details

How it works
------------

The entry for the Pattern Table just works out nicely useful for both modes,
even though it ends up having different meaning!

Multicolor normally assumes each pattern is repeated 4 times, bitmap only 3. 
By using all 256 characters for both, we can share the SIT between them. This
wastes about 512 bytes in the multicolor pattern table, but gain back 640 bytes
for sprites versus a separate table. (The wasted memory is scattered to every 
character definition as two rows in each would not be used).

To take advantage of this shared table, both multicolor and bitmap need to be
laid out a little differently than normal. The benefits in terms of number of
registers to change and memory consolidation seems to be worth it.

Follows is a table showing the starting character in the SIT for each character
row:

---------- Bitmap block 0
 0:	0
 1: 32
 2: 64
 3: 96
 
 4: 128
 5: 160
 6: 192
 7: 224
---------- Bitmap block 1
 8: 32
 9: 64
10: 96
11: 128

12: 160
13: 192
14: 224
15: 0
---------- Bitmap block 2
16: 64
17: 96
18: 128
19: 160

20: 192
21: 224
22: 0
23: 32
----------

Notice how the rotation of the characters requires some adjustment to the bitmap
image data, but allows a proper interleave for the multicolor mode characters.

For multicolor, each group of four loads two bytes per row (representing two vertical 
pixels), at the appropriate offset:

- the first in a group loads bytes 0-1
- the second in a group loads bytes 2-3
- the third in a group loads bytes 4-5
- the fourth in a group loads bytes 6-7

Each value appears only three times, so two bytes in each definition are unused. 
Which two bytes that is varies, though. This program fills in the unused bytes
in order to aid RLE compression.

Scanline Palette Bitmap F18A
----------------------------

This is the recommended VDP memory layout for this mode:

0000-17FF = Pattern table
1800-19FF = Screen Image Table (0-255 repeated 3 times)
1A00-1A1F = Sprite Attribute List (just needs a D0 00 to terminate the sprite list, but all space reserved)
1A20-1FFF = almost 1.5k for program code to display the screen (won't need much, you could also put sprite patterns in here!)
2000-37FF = Color table
3800-47FF = Palette Per Scanline table (note it uses the extended 2k of GPU memory for a full 6k)

Since the GPU can completely handle the palette changes, this mode can be implemented with zero impact
on the console's 9900 CPU, not even an interrupt function is needed.

Some GPU code is needed - a program to change the palette every scanline, and a stub program
to handle moving the palette data into the extended VDP memory. 


Command Line
------------

A simple command line now exists. If you pass two filenames (file_in, file_out), then
file_in will be read and converted, then saved as file_out with the default settings.
A BMP version of the file is also written (so don't use this with BMP files!). 

/verbose - enables more verbose output than normal. Works in GUI and Command Prompt mode.
/? - outputs help

You can also override any setting in the INI file by prefacing it with / like so:
/option=value
When values have been overridden on the command line, the INI is not saved on exit.

Using /verbose will let you see all the options, including which are read from the
command line. Strings are all case-sensitive.


INI File
--------

On exit, all current settings are saved in convert9918.ini. This is still being tested.
Note, however, that the palette may be editted in the INI file. 
The byte order is R,G,B,unused.

//
// (C) 2017 Mike Brent aka Tursi aka HarmlessLion.com
// This software is provided AS-IS. No warranty
// express or implied is provided.
//
// This notice defines the entire license for this software.
// All rights not explicity granted here are reserved by the
// author.
//
// You may redistribute this software provided the original
// archive is UNCHANGED and a link back to my web page,
// http://harmlesslion.com, is provided as the author's site.
// It is acceptable to link directly to a subpage at harmlesslion.com
// provided that page offers a URL for that purpose
//
// Source code, if available, is provided for educational purposes
// only. You are welcome to read it, learn from it, mock
// it, and hack it up - for your own use only.
//
// Please contact me before distributing derived works or
// ports so that we may work out terms. I don't mind people
// using my code but it's been outright stolen before. In all
// cases the code must maintain credit to the original author(s).
//
// Unless you have explicit written permission from me in advance,
// this code may never be used in any situation that changes these
// license terms. For instance, you may never include GPL code in
// this project because that will change all the code to be GPL.
// You may not remove these terms or any part of this comment
// block or text file from any derived work.
//
// -COMMERCIAL USE- Contact me first. I didn't make
// any money off it - why should you? ;) If you just learned
// something from this, then go ahead. If you just pinched
// a routine or two, let me know, I'll probably just ask
// for credit. If you want to derive a commercial tool
// or use large portions, we need to talk. ;)
//
// Commercial use means ANY distribution for payment, whether or
// not for profit.
//
// If this, itself, is a derived work from someone else's code,
// then their original copyrights and licenses are left intact
// and in full force.
//
// http://harmlesslion.com - visit the web page for contact info
//

