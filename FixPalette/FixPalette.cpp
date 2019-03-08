// FixPalette.cpp : Defines the entry point for the console application.
// Just a dirty little tool to fix errors in F18A palette output for the
// last few versions. Do not execute on good pictures, it can't tell
// whether the palette needs the swap or not!

#include "stdafx.h"
#include <string.h>

unsigned char buf[8192];

int main(int argc, wchar_t* argv[])
{
	FILE *fp = NULL;
	if (argc > 1) {
		wchar_t *p = wcsrchr(argv[1], _T('M'));
		if (NULL == p) {
			p = wcsrchr(argv[1], _T('m'));
		}
		if ((NULL == p) || (*(p+1)!=_T('\0'))) {
			printf("\n* You MUST pass in the name of the palette (_M/TIAM) file!\n");
		} else {
			fp=_wfopen(argv[1], L"rb+");
			if (NULL == fp) {
				printf("Can't open file to edit.\n");
			}
		}
	}
	if (NULL == fp) {
		printf("\nExecute only on images from an old version of Convert9918\n");
		printf("which display improper palette on F18A.\n");
		printf("This tool just does a palette fixup - color 14 is\n");
		printf("inadvertantly saved in 2's slot, shifting all the others.\n");
		printf("(It makes more sense in context. ;) )\n");
		printf("This was fixed in Convert9918 on Oct 1 2016 and won't\n");
		printf("fix later images. ;)\n");
		return 1;
	}

	int s = fread(buf, 1, sizeof(buf), fp);
	
	// we expect very specific sizes - won't support RLE (but the tool
	// won't write an RLE palette anyway. I think...)
	int start = 0;

	switch (s) {
	case 32:		// raw static palette
	case 6144:		// raw scanline palette
		break;

	case 32+128:	// static palette with header
	case 6144+128:	// scanline palette with header
		start = 128;
		break;

	default:
		printf("Could not determine palette type by size - RLE not supported.\n");
		printf("Expect 32 or 6144 bytes, or same with 128 byte header.\n");
		return 2;
	}

	if (s < 6144) {
		printf("Fixing static palette...\n");
	} else {
		printf("Fixing scanline palette...\n");
	}

	// loop through and do the fix ups
	while (start < s) {
		int p = start+4;		// color 2
		int x1 = buf[p];
		int x2 = buf[p+1];

		memmove(&buf[p], &buf[p+2], 24);	// 12 colors to move

		p+=24;
		buf[p]=x1;
		buf[p+1]=x2;

		start+=32;
	}

	// and write it back out
	fseek(fp, 0, SEEK_SET);
	fwrite(buf, 1, s, fp);
	fclose(fp);

	return 0;
}

