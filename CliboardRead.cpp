// based on example by Brandon at https://stackoverflow.com/questions/30552255/how-to-read-a-bitmap-from-the-windows-clipboard
// Doesn't handle all possible formats, but it's a start!

#include <iostream>
#include <fstream>
#include <windows.h>
#include "ClipboardRead.h"

// create a temporary file that we'll remember to delete on close
tempfile myTmpFile;

bool clipboardRead()
{
    bool ret = false;
    bool hasDib = IsClipboardFormatAvailable(CF_DIB);
    bool hasBmp = IsClipboardFormatAvailable(CF_BITMAP);
    bool hasDibv5 = IsClipboardFormatAvailable(CF_DIBV5);

    if ((!hasDib)&&(!hasBmp)&&(!hasDibv5)) {
        return false;
    }

#if 0
    std::cout<<"Format Bitmap: "<<hasBmp<<"\n";
    std::cout<<"Format DIB: "<<hasDib<<"\n";
    std::cout<<"Format DIBv5: "<<hasDibv5<<"\n";
#endif

    if (hasDib)
    {
        if (OpenClipboard(NULL))
        {
            HANDLE hClipboard = GetClipboardData(CF_DIB);
            if (hClipboard != NULL && hClipboard != INVALID_HANDLE_VALUE)
            {
                void* dib = GlobalLock(hClipboard);

                if (dib)
                {
                    BITMAPINFOHEADER* info = reinterpret_cast<BITMAPINFOHEADER*>(dib);
                    BITMAPFILEHEADER fileHeader = {0};
                    fileHeader.bfType = 0x4D42;
                    fileHeader.bfOffBits = 54;
                    fileHeader.bfSize = (((info->biWidth * info->biBitCount + 31) & ~31) / 8
                              * info->biHeight) + fileHeader.bfOffBits;

                    // TODO: doesn't handle palettes at all. Probably doesn't handle a lot. Do we need to?

#if 0
                    std::cout<<"Type: "<<std::hex<<fileHeader.bfType<<std::dec<<"\n";
                    std::cout<<"bfSize: "<<fileHeader.bfSize<<"\n";
                    std::cout<<"Reserved: "<<fileHeader.bfReserved1<<"\n";
                    std::cout<<"Reserved2: "<<fileHeader.bfReserved2<<"\n";
                    std::cout<<"Offset: "<<fileHeader.bfOffBits<<"\n";
                    std::cout<<"biSize: "<<info->biSize<<"\n";
                    std::cout<<"Width: "<<info->biWidth<<"\n";
                    std::cout<<"Height: "<<info->biHeight<<"\n";
                    std::cout<<"Planes: "<<info->biPlanes<<"\n";
                    std::cout<<"Bits: "<<info->biBitCount<<"\n";
                    std::cout<<"Compression: "<<info->biCompression<<"\n";
                    std::cout<<"Size: "<<info->biSizeImage<<"\n";
                    std::cout<<"X-res: "<<info->biXPelsPerMeter<<"\n";
                    std::cout<<"Y-res: "<<info->biYPelsPerMeter<<"\n";
                    std::cout<<"ClrUsed: "<<info->biClrUsed<<"\n";
                    std::cout<<"ClrImportant: "<<info->biClrImportant<<"\n";
#endif

                    unsigned int estimatedSize = info->biWidth*info->biHeight*(info->biBitCount/8);
//                    std::cout <<"Estimated data size: " << estimatedSize<<"\n";
//                    std::cout <<"Memory size: " << GlobalSize(dib)-sizeof(BITMAPINFOHEADER)<<"\n";

                    // avoid overrunning the buffer
                    if (estimatedSize <= GlobalSize(dib)-sizeof(BITMAPINFOHEADER)) {
                        std::ofstream file(myTmpFile.getfilename(), std::ios::out | std::ios::binary);
                        if (file)
                        {
                            std::cout << "Writing image from clipboard to " << myTmpFile.getfilename() << std::endl;
                            file.write(reinterpret_cast<char*>(&fileHeader), sizeof(BITMAPFILEHEADER));
                            file.write(reinterpret_cast<char*>(info), sizeof(BITMAPINFOHEADER));
                            file.write(reinterpret_cast<char*>(++info), estimatedSize);
                            ret = true;
                        }
                    }

                    GlobalUnlock(dib);
                }
            }

            EmptyClipboard();
            CloseClipboard();
        }
    }

    return ret;
}