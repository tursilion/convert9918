// Also need to honour the transparent color in animated GIFs if we're gonna play them out ;)

// GIF Loader
// by Paul Bartrum
// Modified by M.Brent

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// Removal of this pack causes crashes and bad decodes
#pragma pack(1)

int _color_load_depth(int depth);

struct LZW_STRING
{
	short base;
	char cnew;
	short length;
};

FILE *f;
int empty_string, curr_bit_size, bit_overflow;
int bit_pos, data_pos, data_len, entire, code;
int cc, string_length, i, bit_size;
unsigned char string[4096];
struct LZW_STRING str[4096];
HGLOBAL bmp;
struct {
	char r, g, b;
} pal[256];
int image_x, image_y, image_w, image_h, x, y;
int width, height, depth;
int interlace, transparent;
UINT32 nMaxBytes;

extern HGLOBAL load_gif(char *filename, unsigned int *iWidth, unsigned int *iHeight);

int myfgetc(FILE *fp) {
	static FILE *handle=0;
	static bool fEOF=false;
	int c;

	if (fp!=handle) {
		fEOF=false;
		handle=fp;
	}

	if (fEOF) {
		return EOF;
	}

	c=fgetc(fp);
	if (EOF == c) fEOF=true;

	return c;
}

void clear_table(void)
{
	empty_string = cc + 2;
	curr_bit_size = bit_size + 1;
	bit_overflow = 0;
}


void get_code(void)
{
	if(bit_pos + curr_bit_size > 8) {
		if(data_pos >= data_len) { data_len = myfgetc(f); data_pos = 0; }
		entire = (myfgetc(f) << 8) + entire;
		data_pos ++;
	}
	if(bit_pos + curr_bit_size > 16) {
		if(data_pos >= data_len) { data_len = myfgetc(f); data_pos = 0; }
		entire = (myfgetc(f) << 16) + entire;
		data_pos ++;
	}
	code = (entire >> bit_pos) & ((1 << curr_bit_size) - 1);
	if(bit_pos + curr_bit_size > 8)
		entire >>= 8;
	if(bit_pos + curr_bit_size > 16)
		entire >>= 8;
	bit_pos = (bit_pos + curr_bit_size) % 8;
	if(bit_pos == 0) {
		if(data_pos >= data_len) { data_len = myfgetc(f); data_pos = 0; }
		entire = myfgetc(f);
		data_pos ++;
	}
}


void get_string(int num)
{
	if(num < cc)
	{
		string_length = 1;
		string[0] = str[num].cnew;
	}
	else
	{
		i = str[num].length;
		string_length = i;
		while(i > 0)
		{
			i --;
			string[i] = str[num].cnew;
			num = str[num].base;
		}
		/* if(num != -1) **-{[ERROR]}-** */
	}
}


void output_string(void)
{
	for(i = 0; i < string_length; i ++)
	{
		// sanity check - ignore out of range values
		if (string[i]>256) string[i]&=0xff;
		if (transparent != string[i]) {
			if ((unsigned)((y*width*3)+(x*3)+2) < nMaxBytes) {
				((char*)bmp)[(y*width*3)+(x*3)]=pal[string[i]].r;
				((char*)bmp)[(y*width*3)+(x*3)+1]=pal[string[i]].g;
				((char*)bmp)[(y*width*3)+(x*3)+2]=pal[string[i]].b;
			}
		}

		x ++;
		if(x >= image_x + image_w)
		{
			x = image_x;
			y += interlace;
			if(interlace)
			{
				if(y >= image_y + image_h)
				{
					if(interlace == 8 && (y - image_y) % 8 == 0) {
						interlace = 8;
						y = image_y + 4;
					}
					else if(interlace == 8  && (y - image_y) % 8 == 4) {
						interlace = 4;
						y = image_y + 2;
					}
					else if(interlace == 4) {
						interlace = 2;
						y = image_y + 1;
					}
				}
			}
		}
	}
}

int pack_mgetw(FILE *f)
{
	int a = myfgetc(f);
	int b = myfgetc(f);

	return (a<<8)|b;
}

int pack_igetw(FILE *f)
{
	int a = myfgetc(f);
	int b = myfgetc(f);

	return a | (b<<8);
}

/* load_gif:
 *  Loads a 2-256 colour GIF file onto a bitmap, returning an RGB buffer
 *  and storing the width and height in the passed pointers
 */
HGLOBAL load_gif(char *filename, unsigned int *iWidth, unsigned int *iHeight)
{
	int old;

	transparent=-1;

	fopen_s(&f, filename, "rb");
	if (!f) /* can't open file */
		return NULL;

	i  = pack_mgetw(f) << 8;
	i += myfgetc(f);
	if(i != 0x474946) /* is it really a GIF? */
	{
		fclose(f);
		return NULL;
	}
	fseek(f, 3, SEEK_CUR); /* skip version */

	width = pack_igetw(f);
	height = pack_igetw(f);

	if (feof(f)) {
		fclose(f);
		return NULL;
	}

	*iWidth=width;
	*iHeight=height;

	nMaxBytes=width*height*3;
	bmp = GlobalAlloc(GPTR, nMaxBytes);
	if(bmp == NULL) {
		fclose(f);
		return NULL;
	}

	i = myfgetc(f);
	if(i & 128) /* no global colour table? */
		depth = (i & 7) + 1;
	else
		depth = 0;

	if (feof(f)) {
		GlobalFree(bmp);
		fclose(f);
		return NULL;
	}

	fseek(f, 2, SEEK_CUR);	/* skip background colour and aspect ratio */

	if(pal && depth) /* only read palette if pal and depth are not 0 */
	{
		for(i = 0; i < (1 << depth); i ++)
		{
			pal[i].r = (char)myfgetc(f);
			pal[i].g = (char)myfgetc(f);
			pal[i].b = (char)myfgetc(f);
		}
	}
	else
		if(depth)
			fseek(f, (1 << depth) * 3, SEEK_CUR);

	if (feof(f)) {
		GlobalFree(bmp);
		fclose(f);
		return NULL;
	}

	for (;;) {
		i = myfgetc(f);
		if (EOF == i) {
			printf(" * Warning - Unexpected end of file!\n");
			GlobalFree(bmp);
			fclose(f);
			return NULL;
		}
		switch(i)
		{
			case 0x2C: /* Image Descriptor */
				image_x = pack_igetw(f);
				image_y = pack_igetw(f); /* individual image dimensions */
				image_w = pack_igetw(f);
				image_h = pack_igetw(f);

				i = myfgetc(f);
				if(i & 64)
					interlace = 8;
				else
					interlace = 1;

				if(i & 128)
				{
					depth = (i & 7) + 1;
					if(pal)
					{
						for(i = 0; i < (1 << depth); i ++)
						{
							pal[i].r = (char)myfgetc(f);
							pal[i].g = (char)myfgetc(f);
							pal[i].b =(char) myfgetc(f);
						}
					}
					else
						fseek(f, (1 << depth) * 3, SEEK_CUR);
				}

				/* lzw stream starts now */
				bit_size = myfgetc(f);
				cc = 1 << bit_size;

				/* initialise string table */
				for(i = 0; i < cc; i ++)
				{
					str[i].base = -1;
					str[i].cnew = (char)i;
					str[i].length = 1;
				}

				/* initialise the variables */
				bit_pos = 0;
				data_len = myfgetc(f); data_pos = 0;
				entire = myfgetc(f); data_pos ++;
				string_length = 0; x = image_x; y = image_y;

				/* starting code */
				clear_table();
				get_code();
				if(code == cc)
					get_code();
				get_string(code);
				output_string();
				old = code;

				while(!feof(f))
				{
					get_code();

					if(code == cc)
					{
						/* starting code */
						clear_table();
						get_code();
						get_string(code);
						output_string();
						old = code;
					}
					else if(code == cc + 1)
					{
						break;
					}
					else if(code < empty_string)
					{
						get_string(code);
						output_string();

						if(bit_overflow == 0) {
							str[empty_string].base = (short)old;
							str[empty_string].cnew = string[0];
							str[empty_string].length = str[old].length + 1;
							empty_string ++;
							if(empty_string == (1 << curr_bit_size))
								curr_bit_size ++;
							if(curr_bit_size == 13) {
								curr_bit_size = 12;
								bit_overflow = 1;
							}
						}

						old = code;
					}
					else
					{
						get_string(old);
						string[str[old].length] = string[0];
						string_length ++;

						if(bit_overflow == 0) {
							str[empty_string].base = (short)old;
							str[empty_string].cnew = string[0];
							str[empty_string].length = str[old].length + 1;
							empty_string ++;
							if(empty_string == (1 << curr_bit_size))
								curr_bit_size ++;
							if(curr_bit_size == 13) {
								curr_bit_size = 12;
								bit_overflow = 1;
							}
						}

						output_string();
						old = code;
					}
				}
				
				// We only want the first image in the file
				fclose(f);
				return bmp;

				break;
			case 0x21: /* Extension Introducer */
				i = myfgetc(f);
				if(i == 0xF9) /* Graphic Control Extension */
				{
					fseek(f, 1, SEEK_CUR); /* skip size (it's 4) */
					i = myfgetc(f);
					if(i & 1) /* is transparency enabled? */
					{
						fseek(f, 2, SEEK_CUR);
						transparent=myfgetc(f); /* transparent colour */
					}
					else
						fseek(f, 3, SEEK_CUR);
				}
				i = myfgetc(f);
				while(i) /* skip Data Sub-blocks */
				{
					fseek(f, i, SEEK_CUR);
					i = myfgetc(f);
				}
				break;
			case 0x3B: /* Trailer - end of data */
				fclose(f);

				return bmp;
		}
	}
}
