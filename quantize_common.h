// The following parts are the same for all the quantize methods

extern char *cmdFileIn;
extern bool fVerbose;
extern MYRGBQUAD palinit16[256];
extern float_precision g_thresholdMap2x2[2][2];
extern float_precision g_thresholdMap4x4[4][4];

// macro to change the masking for the threshold map
#define MAPSEEK(x) (x)&maskval

// pIn is a 24-bit RGB image
// pOut is an 8-bit palettized image, with the palette in 'pal'
// pOut contains a multicolor mapped image if half-multicolor is on
// CurrentBest is the current closest value and is used to abort a search without
// having to check all 8 pixels for a small speedup when the color is close
// note that this function is never compiled -- it is renamed by each various function define
void quantize_common(BYTE* pRGB, BYTE* p8Bit, double darkenval, int mapSize) {
	int row,col;
	// local temporary usage - adjusted map
	float_precision g_thresholdMap2[4][4];
	float_precision nDesiredPixels[8][3];		// there are 8 RGB pixels to match
#ifdef ERROR_DISTRIBUTION
	float_precision fPIXA, fPIXB, fPIXC, fPIXD, fPIXE, fPIXF;
#endif
	int nFixedColors = 0;				// used only when perscanlinepalette is set
	float_precision (*thresholdMap)[4];
	int maskval;

	if (mapSize == 2) {
		maskval = 1;
	} else if (mapSize == 4) {
		maskval = 3;
	} else {
		printf("Bad map size %d\n", mapSize);
		return;
	}

	// update threshold map 2 for the darken value
	for (int i1=0; i1<mapSize; i1++) {
		for (int i2=0; i2<mapSize; i2++) {
			// so it's a slider now - but the original algorithm /does/ center around zero
			// so normally this should be at 8, halfway
			// Since this is a fraction it doesn't matter if it's 8x8 or 4x4
			if (mapSize == 2) {
				g_thresholdMap2[i1][i2] = g_thresholdMap2x2[i1][i2] - (darkenval/16.0);
			} else {
				g_thresholdMap2[i1][i2] = g_thresholdMap4x4[i1][i2] - (darkenval/16.0);
			}
		}
		thresholdMap = g_thresholdMap2;
	}

	// timing report
	time_t nStartTime, nEndTime;
	time(&nStartTime);

#ifdef ERROR_DISTRIBUTION
	// convert the integer dither ratios just once up here
	fPIXA = (float_precision)PIXA/16.0;
	fPIXB = (float_precision)PIXB/16.0;
	fPIXC = (float_precision)PIXC/16.0;
	fPIXD = (float_precision)PIXD/16.0;
	fPIXE = (float_precision)PIXE/16.0;
	fPIXF = (float_precision)PIXF/16.0;
#endif

#ifdef QUANTIZE_FLICKER
	// square the half-multicolor ratio so we can ignore signs, and scale up to a 0-255 range
	float_precision ratio = (float_precision)((g_MaxMultiDiff*2.55)*(g_MaxMultiDiff*2.55));
#endif

	// create some workspace
#ifdef ERROR_DISTRIBUTION
	float_precision *pError = (float_precision*)malloc(sizeof(float_precision)*258*194*3);	// error map, 3 colors, includes 1 pixel border on sides & 2 on bottom (so first entry is x=-1, y=0, and each row is 258 pixels wide)
	for (int idx=0; idx<258*194*3; idx++) {
		pError[idx]=0.0;
	}
#endif

	if (g_UsePalette) {
		nFixedColors = 15;
		if (g_UsePerLinePalette) {
			nFixedColors = g_StaticColors;
		}
	}

	// check if we need to create a new palette
	if (g_UsePalette) {
		// do we need to calculate any fixed colors?
		if (nFixedColors > 0) {
			if (g_bStaticByPopularity) {
				debug("Preserving %d top colors (popularity)\n", nFixedColors);

				int nColCount = 0;

				// if we are doing a per-line palette, fix the 4* most popular colors to reduce horizontal lines
				// rather than 'first' color, we should probably save 2-4 colors by popularity and
				// only change the rest. 
				// I tried keeping the 4 most DISTINCT colors per line, that was worse than this approach.
                // * - not 4 anymore, user specified, and usually zero these days
				memset(cols, 0, sizeof(cols));
				for (row=0; row<192; row++) {
					for (col=0; col<256; col++) {
						// address of byte in the RGB image (3 bytes per pixel)
						BYTE *pInLine = pRGB + (row*256*3) + (col*3);
						// make a 12-bit color, rounding colors up
						int idx= ((MakeRoundedRGB(*pInLine)&0xf0)<<4) |
								 ((MakeRoundedRGB(*(pInLine+1)))&0xf0) |
								 ((MakeRoundedRGB(*(pInLine+2))&0xf0)>>4);
						cols[idx]++;
					}
				}

				// now we've counted them, choose the top x
				int top[16], topcnt[16];
				for (int idx=0; idx<16; idx++) {
					top[idx]=0;
					topcnt[idx]=0;
				}
				for (int idx=0; idx<4096; idx++) {
					if (cols[idx] > 0) nColCount++;		// just some stats since we're here

					for (int idx2=0; idx2<16; idx2++) {
						if (cols[idx] > topcnt[idx2]) {
							top[idx2]=idx;
							topcnt[idx2]=cols[idx];
							break;
						}
					}
				}
				debug("%d relevant colors detected (%d%%).\n", nColCount, nColCount*100/4096);

				// should have (up to) 15 colors sorted now, grab the top x (popularity)
				for (int idx=0; idx<nFixedColors; idx++) {
					pal[idx+1][0]=(((top[idx]&0xf00)>>8)<<4)+8;		// +8 to center the color in the middle of the range (0x?0 - 0x?F)
					pal[idx+1][1]=(((top[idx]&0xf0)>>4)<<4)+8;
					pal[idx+1][2]=(((top[idx]&0xf))<<4)+8;

					makeYUV(pal[idx+1][0], pal[idx+1][1], pal[idx+1][2], YUVpal[idx+1][0], YUVpal[idx+1][1], YUVpal[idx+1][2]);
				}
			} else {
				debug("Preserving %d top colors (median cut)\n", nFixedColors);
				// need to make a work copy of the image
				// this uses median cut over the entire image down to the number of static colors
				unsigned char *pWork = (unsigned char*)malloc(256*192*3);
				memcpy(pWork, pRGB, 256*192*3);
				// to force 4 bit color guns in medianCut, reduce the image to 4 bits
				// otherwise we get duplicate colors. Since it's a work buffer, we don't
				// need to shift it back up, but we do need to shift up the colors
				unsigned char *pTmp = pWork;
				for (int idx=0; idx<256*192*3; idx++) {
					*(pTmp++)>>=4;
				}
				std::list<Point> newpal = medianCut((Point*)pWork, 256*192, nFixedColors);
				free(pWork);
				// copy the returned palette back as fixed colors
				std::list<Point>::iterator iter;
				int idx = 0;
				for (iter = newpal.begin() ; iter != newpal.end(); iter++) {
					pal[idx][0]=MakeRoundedRGB(iter->x[0]<<4);
					pal[idx][1]=MakeRoundedRGB(iter->x[1]<<4);
					pal[idx][2]=MakeRoundedRGB(iter->x[2]<<4);

					makeYUV(pal[idx][0], pal[idx][1], pal[idx][2], YUVpal[idx][0], YUVpal[idx][1], YUVpal[idx][2]);

					idx++;
				}
			}
		}
	}

	// TODO: tough and tricky maybe to try someday -- 24 colors per scanline
	// can be done by dividing the scanline into 2 x 128 pixel sections, and processing
	// each separately. Instead of a 16 color palette per scanline, update only 8 colors
	// per segment (keeping the rest). Start with an initial 16-color palette defined
	// for the first segment (8 colors extra needed plus the 8 for that segment). To
	// indicate which colors are replaced, a 16-bit word is used - set bits replace that
	// color in the palette with the next of the new 8 colors. Replace the 8 colors
	// furthest from correct in the block. Cost will be an extra 16 bytes for the
	// 8 preloaded colors and an extra 384 bytes for the palette index bits. (400 bytes
	// total). Should be enough space left, I think! (But same issue as HAM4, we can't
	// get accurate enough start timing on the GPU to trigger reliably.)
	// TODO: but we might have that now... we have hblank trigger for the GPU

	// go ahead and get to work
	for (row = 0; row < 192; row++) {
		MSG msg;
		if (row%8 == 0) {
			// this one debug, I don't want to go to outputdebugstring for performance reasons...
			if ((cmdFileIn == NULL) || (fVerbose)) {
				printf("Processing row %d...\r", row);
			}

			// keep windows happy
			if (NULL == cmdFileIn) {
				if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE|PM_NOYIELD)) {
					AfxGetApp()->PumpMessage();
				}
			}
		}

#ifdef ERROR_DISTRIBUTION
		// divisor for the error value. Three errors are added together except for the left
		// column (only 2), and top row (only 1). The top left pixel has none, but no divisor
		// is needed since the stored value is also zero.
		// there is no notable performance difference to removing this for the accumulate errors mode
		float_precision nErrDivisor = 3.0;
		if ((row == 0)||(g_AccumulateErrors)) nErrDivisor=1.0;
#endif

		if (g_UsePalette) {
			int nCol = 0;

			if (g_UsePerLinePalette) {
				// Also - could we reliably change the palette more than once per line? Is there a horizontal reg?
				// Spectrum 512 on the ST changed 3 times across the 320 pixel line.
				// (yes there is - but where can we store the color data? We are out of VRAM).
			
				// I tried two lines at once to reduce banding, the results were slightly worse by losing detail and increasing
				// the size of error patches. So don't do that.
				memcpy(points, (pRGB+(row*256*3)), 256*3);

				// I can't tell if this is better in or out.
				// add in the error if not row 0

				if (row > 0) {
					// address of entry in the error table (3 doubles per pixel and larger array)
#ifdef ERROR_DISTRIBUTION
					float_precision *pErrLine = pError + (row*258*3) + 3;
#endif
					for (int idx=0; idx<256*3; idx++) {	// we can loop over R,G,B in sequence for this
						int val = points[idx/3].x[idx%3];
#ifdef QUANTIZE_ORDERED
						val += (int)(val*thresholdMap[MAPSEEK(idx)][MAPSEEK(row)]);
#endif
#ifdef ERROR_DISTRIBUTION
						val	+= (int)((*pErrLine)/nErrDivisor+0.5);
#endif

						// clamp - it's necessary cause we're going into a byte, but it's technically wrong
						if (val > 255) val=255;
						if (val < 0) val=0;
						points[idx/3].x[idx%3] = val;
					}
				}

				// since we have a set of fixed colors, don't include them in the set of palette selection pixels (better chance at a good palette)
				// So first, find a color not in the list of fixed colors that we can actually use
				if (nFixedColors>0) {
					for (int idx=0; idx<256; idx++) {
						nCol = idx;
						for (int i2=0; i2<nFixedColors; i2++) {
							if ((MakeRoundedRGB(points[idx].x[0]) == pal[i2][0]) &&
								(MakeRoundedRGB(points[idx].x[1]) == pal[i2][1]) &&
								(MakeRoundedRGB(points[idx].x[2]) == pal[i2][2])) {
									// not this one
									nCol=-1;
									break;
							}
						}
						if (nCol != -1) {
							// this works!
							break;
						}
					}

					if (nCol != -1) {
						// we did find one. If we didn't, then it doesn't really matter what the rest of the palette does,
						// we can skip the median cut, really.
						// but in this case, remap any such colors to the one we found
                        // TODO: this would impact median cut and popularity by skewing that one color's value...
                        // But maybe we can just delete the fixed color concept anyway now...
						for (int idx=0; idx<256; idx++) {
							for (int i2=0; i2<nFixedColors; i2++) {
								if ((MakeRoundedRGB(points[idx].x[0]) == pal[i2][0]) &&
									(MakeRoundedRGB(points[idx].x[1]) == pal[i2][1]) &&
									(MakeRoundedRGB(points[idx].x[2]) == pal[i2][2])) {
										// remap this one
										points[idx] = points[nCol];
										break;
								}
							}
						}
					}
				}

				// if nCol is -1, then all pixels on the line are one of the fixed colors. It will be 0 if there are no fixed colors.
				if ((nFixedColors<15) && (nCol != -1)) {
					// things tried and abandoned:
					// -median cut
					// -2 line popularity
					// this version merges the closest colors until we only have the correct number left
					// but it weights them by local popularity, including the line above
                    // This is not too bad...
					float_precision pop[256];		// popularity of each pixel in the row (or 3 rows in the other case)
					// in the first pass, each pixel gets 3 points, plus bonus based on how close to the pixels above it is
					// we weight this pixel with the rows above and below, like so:
					float_precision maxdist = 256.0*256.0+256.0*256.0+256.0*256.0;
					maxdist=sqrt((double)maxdist);
					for (int idx=0; idx<256; idx++) {
						// basically increases the popularity of 'this' pixel by how close it is to the ones near it
						// 1 2 1
						//   3
						// 1 2 1
						// calculate popularity on this row before adding offsets
						pop[idx]=3.0;	// will become 3.0 when we add the pixel it actually is
						// now check the neighboring rows
						if (row>0) {
							int off=((row-1)*256*3)+(idx-1)*3;	// up and to the left
							if (idx > 0) {
								float_precision dist = sqrt((double)yuvdist(*(pRGB+off), *(pRGB+off+1), *(pRGB+off+2), 
															points[idx].x[0], points[idx].x[1], points[idx].x[2])) / maxdist;	// 0.0-1.0
								pop[idx]+=1.0-dist;
							}

							{
								float_precision dist = sqrt((double)yuvdist(*(pRGB+off+3), *(pRGB+off+4), *(pRGB+off+5), 
															points[idx].x[0], points[idx].x[1], points[idx].x[2])) / (maxdist/2.0); // 0.0-2.0
								pop[idx]+=2.0-dist;
							}

							if (idx < 255) {
								float_precision dist = sqrt((double)yuvdist(*(pRGB+off+6), *(pRGB+off+7), *(pRGB+off+8), 
															points[idx].x[0], points[idx].x[1], points[idx].x[2])) / maxdist; // 0.0-1.0
								pop[idx]+=1.0-dist;
							}
						}
						if (row<191) {
							int off=((row+1)*256*3)+(idx-1)*3;	// down and to the left
							if (idx > 0) {
								float_precision dist = sqrt((double)yuvdist(*(pRGB+off), *(pRGB+off+1), *(pRGB+off+2), 
															points[idx].x[0], points[idx].x[1], points[idx].x[2])) / maxdist; // 0.0-1.0
								pop[idx]+=1.0-dist;
							}

							{
								float_precision dist = sqrt((double)yuvdist(*(pRGB+off+3), *(pRGB+off+4), *(pRGB+off+5), 
															points[idx].x[0], points[idx].x[1], points[idx].x[2])) / (maxdist/2.0); // 0.0-2.0
								pop[idx]+=2.0-dist;
							}

							if (idx < 255) {
								float_precision dist = sqrt((double)yuvdist(*(pRGB+off+6), *(pRGB+off+7), *(pRGB+off+8), 
															points[idx].x[0], points[idx].x[1], points[idx].x[2])) / maxdist; // 0.0-1.0
								pop[idx]+=1.0-dist;
							}
						}
					}
					// in the second pass, we look for identical pixels down the way and remove them from the pop list
					int nCols=256;
					for (int idx=0; idx<256; idx++) {
						for (int idx2=idx+1; idx2<256; idx2++) {
							if ((pop[idx]==0)||(pop[idx2]==0)) continue;

                            // we compare exact match
							if ( ((points[idx].x[0]) == (points[idx2].x[0])) && 
								 ((points[idx].x[1]) == (points[idx2].x[1])) &&
								 ((points[idx].x[2]) == (points[idx2].x[2])) ) {
								pop[idx]+=pop[idx2];
								pop[idx2]=0;
								nCols--;
							}
						}
					}

					while (nCols > 15-nFixedColors) {
						// choose two colors to merge by their distance weighted by popularity scores (more popular makes farther apart)
						float_precision mindist=0x7fffffff;
						int mindistp1=0, mindistp2=0;
						for (int idx=0; idx<256; idx++) {
							if (pop[idx]==0) continue;
							for (int idx2=idx+1; idx2<256; idx2++) {
								if (pop[idx2]==0) continue;

								float_precision dist = yuvdist(points[idx].x[0], points[idx].x[1], points[idx].x[2], points[idx2].x[0], points[idx2].x[1], points[idx2].x[2]) * pop[idx] * pop[idx2];

								if (dist < mindist) {
									mindistp1=idx;
									mindistp2=idx2;
									mindist=dist;
								}
							}
						}

						// make a new color out of the two we chose - we use the two pop points to scale them
						points[mindistp1].x[0] = (unsigned char)(((points[mindistp2].x[0]*pop[mindistp2])+(points[mindistp1].x[0]*pop[mindistp1]))/(pop[mindistp1]+pop[mindistp2]) + .5);
						points[mindistp1].x[1] = (unsigned char)(((points[mindistp2].x[1]*pop[mindistp2])+(points[mindistp1].x[1]*pop[mindistp1]))/(pop[mindistp1]+pop[mindistp2]) + .5);
						points[mindistp1].x[2] = (unsigned char)(((points[mindistp2].x[2]*pop[mindistp2])+(points[mindistp1].x[2]*pop[mindistp1]))/(pop[mindistp1]+pop[mindistp2]) + .5);
						// mark the other merged - can't use it
						pop[mindistp1]+=pop[mindistp2];
						pop[mindistp2]=0;
						nCols--;
						// check if we created a color that already exists - if so, nuke this one too
						for (int idx=0; idx<256; idx++) {
							if (pop[idx] == 0) continue;
							if (idx == mindistp1) continue;
							if ( (MakeRoundedRGB(points[mindistp1].x[0]) == MakeRoundedRGB(points[idx].x[0])) && 
								 (MakeRoundedRGB(points[mindistp1].x[1]) == MakeRoundedRGB(points[idx].x[1])) &&
								 (MakeRoundedRGB(points[mindistp1].x[2]) == MakeRoundedRGB(points[idx].x[2])) ) {
                                    float_precision oldp = pop[idx];
									pop[idx]+=pop[mindistp1];	// we can't load into mindistp1 because there may be others
									pop[mindistp1]=0;
									nCols--;
									break;
							}
						}
					}
					// save off the final colors - this gets us down to 15-nFixed final colors from 256
					int idx=g_StaticColors; 
					for (int idx2=0; idx2<256; idx2++) {
						if (pop[idx2]==0) continue;     // skip colors marked unused
						pal[idx][0]=MakeRoundedRGB(points[idx2].x[0]);
						pal[idx][1]=MakeRoundedRGB(points[idx2].x[1]);
						pal[idx][2]=MakeRoundedRGB(points[idx2].x[2]);
						makeYUV(pal[idx][0], pal[idx][1], pal[idx][2], YUVpal[idx][0], YUVpal[idx][1], YUVpal[idx][2]);
						idx++;
					}
				}
			}

#if 0
            // TODO: this makes it streaky!
            // Now that we selected a palette via true color, convert the palette to 12 bits
            // so that we can dither the image correctly.
            // Assuming a linear spread from 0.0 to 1.0 with 16 steps, then converted back to
            // 8 bits, the actual 12-bit palette values map back to 24-bit with each gun with the
            // least significant nibble zeroed, as long as you don't mind >F becomes >FF instead of >100
            // this means it's safe to reconvert even if a value was already 12 bits, at least
			for (int idx=0; idx<16; idx++) {
				pal[idx][0]=MakeTrulyRoundedRGB(pal[idx][0]);
				pal[idx][1]=MakeTrulyRoundedRGB(pal[idx][1]);
				pal[idx][2]=MakeTrulyRoundedRGB(pal[idx][2]);
				makeYUV(pal[idx][0], pal[idx][1], pal[idx][2], YUVpal[idx][0], YUVpal[idx][1], YUVpal[idx][2]);
			}
#endif

			// save this entry off
			memcpy(&scanlinepal[row], pal, sizeof(pal[0][0])*4*16); // 4 for RGB0, 16 for 16 colors
			// copy into winpal for display
			for (int idx=0; idx<16; idx++) {
				winpal[idx].rgbBlue = pal[idx][2];
				winpal[idx].rgbGreen = pal[idx][1];
				winpal[idx].rgbRed = pal[idx][0];
			}
		}
		// the rest as normal

        for (col = 0; col < 256; col+=8) {
			// address of byte in the RGB image (3 bytes per pixel)
			BYTE *pInLine = pRGB + (row*256*3) + (col*3);
			// address of byte in the 8-bit image (1 byte per pixel)
			BYTE *pOutLine = p8Bit + (row*256) + col;

#ifdef ERROR_DISTRIBUTION
			// address of entry in the error table (3 doubles per pixel and larger array)
			float_precision *pErrLine = pError + (row*258*3) + ((col+1)*3);
#endif

			// our first job is to get the desired pixel pattern for this block of 8 pixels
			// This takes the original image, and adds in the data from the error map. We do
			// all matching in float_precision mode now.
			for (int c=0; c<8; c++, pInLine+=3
#ifdef ERROR_DISTRIBUTION
				, pErrLine+=3
#endif
				) {
				nDesiredPixels[c][0] = (float_precision)(*pInLine);		// red
				nDesiredPixels[c][1] = (float_precision)(*(pInLine+1));	// green
				nDesiredPixels[c][2] = (float_precision)(*(pInLine+2));	// blue

				// don't dither if the desired color is pure black or white (helps reduce error spread)	
				if ((nDesiredPixels[c][0]<=8)&&(nDesiredPixels[c][1]<=8)&&(nDesiredPixels[c][2]<=8)) continue;
				if ((nDesiredPixels[c][0]>=0xf8)&&(nDesiredPixels[c][1]>=0xf8)&&(nDesiredPixels[c][2]>=0xf8)) continue;

#ifdef QUANTIZE_ORDERED
				nDesiredPixels[c][0]+=nDesiredPixels[c][0]*thresholdMap[MAPSEEK(c)][MAPSEEK(row)];
				nDesiredPixels[c][1]+=nDesiredPixels[c][1]*thresholdMap[MAPSEEK(c)][MAPSEEK(row)];
				nDesiredPixels[c][2]+=nDesiredPixels[c][2]*thresholdMap[MAPSEEK(c)][MAPSEEK(row)];
#endif
#ifdef ERROR_DISTRIBUTION
				// add in the error table for these pixels
				// Technically pixel 0 on each row except 0 should be /2, this will do /3, but that is close enough
				nDesiredPixels[c][0]+=(*pErrLine)/nErrDivisor;		// we don't want to clamp - that induces color shift
				nDesiredPixels[c][1]+=(*(pErrLine+1))/nErrDivisor;
				nDesiredPixels[c][2]+=(*(pErrLine+2))/nErrDivisor;
#endif
			}

			// tracks the best distance (starts out bigger than expected)
			float_precision nBestDistance = 256.0*256.0*256.0*256.0;
			int nBestFg=0;
			int nBestBg=0;
			int nBestPat=0;

#ifdef ERROR_DISTRIBUTION
			float_precision nErrorOutput[6];				// saves the error output for the next horizontal pixel (vertical only calculated on the best match)
			// zero the error output (will be set to the best match only)
			for (int i1=0; i1<6; i1++) {
				nErrorOutput[i1]=0.0;
			}
#endif

			// start searching for the best match
			int fgmax;
			if (g_UseColorOnly) {
				fgmax = 1;	// no searching foreground
			} else if (g_MatchColors == 2) {
				fgmax = 1;	// only monochrome
			} else {
				fgmax = g_MatchColors;	// whatever we were asked for
			}
			for (int fg=0; fg<fgmax; fg++) {							// foreground color
				// happens 15 times
				for (int bg=fg+1; bg<g_MatchColors; bg++) {						// background color
					// happens 120 times (at most)

#ifdef QUANTIZE_FLICKER
					// we can check right away in half-multicolor whether to allow this for flicker reasons
					// we have to check both foreground and background against both of the affected multicolor pixels
					int pix1=pOutLine[0]&0x0f;
					int pix2=pOutLine[4]&0x0f;
					uchar *pPal1=pal[(pix1<<4)|pix1];	// get the actual pixel color, unmodified
					uchar *pPal2=pal[(pix2<<4)|pix2];	// by mixing it with itself
					uchar *pPal3=pal[(fg<<4)|fg];
					uchar *pPal4=pal[(bg<<4)|bg];
					// luminence always uses the 'perceptual' mapping (but this is NOT configurable)
					float_precision lum1=pPal1[0]*0.30+pPal1[1]*0.59+pPal1[2]*0.11;
					float_precision lum2=pPal2[0]*0.30+pPal2[1]*0.59+pPal2[2]*0.11;
					float_precision lum3=pPal3[0]*0.30+pPal3[1]*0.59+pPal3[2]*0.11;
					float_precision lum4=pPal4[0]*0.30+pPal4[1]*0.59+pPal4[2]*0.11;
					// and finally, check the differences against the maximum scale
					if ((lum1*lum1)-(lum3*lum3) > ratio) continue;
					if ((lum1*lum1)-(lum4*lum4) > ratio) continue;
					if ((lum2*lum2)-(lum3*lum3) > ratio) continue;
					if ((lum2*lum2)-(lum4*lum4) > ratio) continue;
#endif

					int patstart = 0;
					int patend = 256;
					if (g_UseColorOnly) {
						// only legal pattern will be 0x00, foreground won't matter
						patend = 1;
					}
					for (int pat=patstart; pat<patend; pat++) {
						// happens 30,705 times
						float_precision t1,t2,t3;
						float_precision nCurDistance;
						int nMask=0x80;

#ifdef ERROR_DISTRIBUTION
						float_precision tmpR, tmpG, tmpB, farR, farG, farB;
						tmpR=0.0;	// these are used before being overwritten, so they must be initialized
						tmpG=0.0;
						tmpB=0.0;
						farR=0.0;
						farG=0.0;
						farB=0.0;
#endif
						
						// begin the search
						nCurDistance = 0.0;

						for (int bit=0; bit<8; bit++) {					// pixel index into pattern
							// happens 245,640 times
							int nCol;
							// which color to use

#ifdef QUANTIZE_FLICKER
							if (pat&nMask) {
								// use FG color
								nCol = (fg<<4)|(pOutLine[bit]&0x0f);
							} else {
								nCol = (bg<<4)|(pOutLine[bit]&0x0f);
							}
#else
							if (pat&nMask) {
								// use FG color
								nCol = fg;
							} else {
								nCol = bg;
							}
#endif

							// calculate actual error for this pixel
							// Note: wrong divisor for pixel 0 on each row except 0 (should be 2 but will be 3), 
							// but we'll live with it for now

							// update the total error
							float_precision r,g,b;
							// get RGB
							r=nDesiredPixels[bit][0];
							g=nDesiredPixels[bit][1];
							b=nDesiredPixels[bit][2];

#ifdef ERROR_DISTRIBUTION
							// get RGB
							r+=tmpR/nErrDivisor;
							g+=tmpG/nErrDivisor;
							b+=tmpB/nErrDivisor;
#endif

							// we need the RGB diffs for the error diffusion anyway
							t1=r-pal[nCol][0];
							t2=g-pal[nCol][1];
							t3=b-pal[nCol][2];

#ifdef QUANTIZE_PERCEPTUAL
							nCurDistance+=(t1*t1)*g_PercepR+(t2*t2)*g_PercepG+(t3*t3)*g_PercepB;
#else
							nCurDistance+=yuvpaldist(r,g,b, nCol);
#endif

							if (nCurDistance >= nBestDistance) {
								// no need to search this one farther
								break;
							}

#ifdef ERROR_DISTRIBUTION
							// update expected horizontal error based on new actual error
							tmpR=t1*fPIXD;
							tmpG=t2*fPIXD;
							tmpB=t3*fPIXD;

							// migrate the far pixel in
							tmpR+=farR;
							tmpG+=farG;
							tmpB+=farB;

							// calculate the next far pixel
							farR=t1*fPIXE;
							farG=t2*fPIXE;
							farB=t3*fPIXE;
#endif

							// next bit position
							nMask>>=1;
						}

						// this byte is tested, did we find a better match?
						if (nCurDistance < nBestDistance) {
							// we did! So save the data off
							nBestDistance = nCurDistance;
#ifdef ERROR_DISTRIBUTION
							nErrorOutput[0] = tmpR;
							nErrorOutput[1] = tmpG;
							nErrorOutput[2] = tmpB;
							nErrorOutput[3] = farR;
							nErrorOutput[4] = farG;
							nErrorOutput[5] = farB;
#endif
							nBestFg=fg;
							nBestBg=bg;
							nBestPat=pat;
						}
					}
				}
			}

			// at this point, we have a best match for this byte
#ifdef ERROR_DISTRIBUTION
			// first we update the error map to the right with the error output
			*(pErrLine)+=nErrorOutput[0];
			*(pErrLine+1)+=nErrorOutput[1];
			*(pErrLine+2)+=nErrorOutput[2];
			*(pErrLine+3)+=nErrorOutput[3];
			*(pErrLine+4)+=nErrorOutput[4];
			*(pErrLine+5)+=nErrorOutput[5];

			// point to the next line, same x pixel as the first one here
			pErrLine += (258*3) - (8*3);
#endif

			// now we have to output the pixels AND update the error map
			int nMask = 0x80;
			for (int bit=0; bit<8; bit++) {
				int nCol;
#ifdef ERROR_DISTRIBUTION
				float_precision err;
#endif

#ifdef QUANTIZE_FLICKER
				if (nBestPat&nMask) {
					nCol = ((*pOutLine)&0x0f) | (nBestFg<<4);
				} else {
					nCol = ((*pOutLine)&0x0f) | (nBestBg<<4);
				}
#else
				if (nBestPat&nMask) {
					nCol = nBestFg;
				} else {
					nCol = nBestBg;
				}
#endif

				nMask>>=1;

				*(pOutLine++) = nCol;

#ifdef ERROR_DISTRIBUTION
				// and update the error map 
				err = nDesiredPixels[bit][0] - pal[nCol][0];		// red
				*(pErrLine+(bit*3)-3) += err*fPIXA;
				*(pErrLine+(bit*3)) += err*fPIXB;
				*(pErrLine+(bit*3)+3) += err*fPIXC;
				*(pErrLine+(bit*3)+(258*3)) += err*fPIXF;

				err = nDesiredPixels[bit][1] - pal[nCol][1];		// green
				*(pErrLine+(bit*3)-2) += err*fPIXA;
				*(pErrLine+(bit*3)+1) += err*fPIXB;
				*(pErrLine+(bit*3)+4) += err*fPIXC;
				*(pErrLine+(bit*3)+(258*3)+1) += err*fPIXF;

				err = nDesiredPixels[bit][2] - pal[nCol][2];		// blue
				*(pErrLine+(bit*3)-1) += err*fPIXA;
				*(pErrLine+(bit*3)+2) += err*fPIXB;
				*(pErrLine+(bit*3)+5) += err*fPIXC;
				*(pErrLine+(bit*3)+(258*3)+2) += err*fPIXF;
#endif
			}
		}

		// one line complete - draw it to provide feedback
		if ((NULL != pWnd)&&(NULL == cmdFileIn)) {
			CDC *pCDC=pWnd->GetDC();
			if (NULL != pCDC) {
				int dpi = GetDpiForWindow(pWnd->GetSafeHwnd());
				IS40_StretchDraw8Bit(*pCDC, p8Bit+(row*256), 256, 1, 256, winpal, DPIFIX(XOFFSET), DPIFIX(row*2), DPIFIX(256*2), DPIFIX(2));
				if (g_bDisplayPalette) {
					char paltmp[16];
					// draw the palette at the end so we can see it
					for (int idx=0; idx<15; idx++) {
						paltmp[idx] = idx;
					}
					IS40_StretchDraw8Bit(*pCDC, (BYTE*)paltmp, 15, 1, 16, winpal, DPIFIX(XOFFSET+256*2), DPIFIX(row*2), DPIFIX(15*2), DPIFIX(2));
				}
				pWnd->ReleaseDC(pCDC);
			}
		}
	}

#ifdef ERROR_DISTRIBUTION
	// finished! clean up
	free(pError);
#endif

	time(&nEndTime);
	debug("Conversion time: %d seconds\n", nEndTime-nStartTime);
}

