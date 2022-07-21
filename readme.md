v183

This program can convert most modern graphics into a form compatible with the TMS9918A bitmap mode. It supports a drag-and-drop interface and handles resizing and scaling, several forms of user-configurable dithering, adjustable color selection, and more. 

It also supports multi-color (64x48) mode, a dual-multicolor mode with flicker, and a half-multicolor mode that flickers multicolor with bitmap to get more colors. 

Finally, it also supports two palette modes for the F18A VDP replacement - a 16-color palette mode, and a per-scanline palette mode (this mode uses most of the VDP RAM and requires the GPU to update the palette every scanline, but because it runs on the GPU the host CPU is not impacted at all!) 

While it supports writing raw data for pattern and color tables, it also supports writing MSX C2, Coleco CVPaint, Adam PowerPaint, ColecoVision ROM, and TI Extended BASIC images.
