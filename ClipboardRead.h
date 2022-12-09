#pragma once

// yay rai!
class tempfile {
public:
    tempfile() {
        TCHAR path[MAX_PATH+1];

        DWORD ret = GetTempPath(MAX_PATH+1, path);
        if ((ret == 0)||(ret > MAX_PATH+1)) {
            lstrcpy(fn, L".\\c99tmpfile.bmp");
            return;
        }
        lstrcpy(fn, path);
        UINT x = GetTempFileName(path, L"c99", 0, fn);
        if (x == 0) {
            lstrcpy(fn, L".\\c99tmpfile.bmp");
            return;
        }
        int n = lstrlen(fn);
        if ((fn[n-4] == L'.')&&(fn[n-3] == L'T')) {
            // expected to end with ".TMP", change to ".BMP"
            fn[n-3] = L'B';
        }
    }
    ~tempfile() {
        DeleteFile(fn);
    }
    LPCSTR getfilename() { return (LPCSTR)fn; }

private:
    TCHAR fn[MAX_PATH+1];
};

bool clipboardRead();

extern tempfile myTmpFile;
