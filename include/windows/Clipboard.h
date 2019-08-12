#pragma once
#include "Clipboard_Lite.h"

//#include <functional>
#include <string>

#define WIN32_LEAN_AND_MEAN
//#include <atomic>
//#include <mutex>
#include <tchar.h>
//#include <thread>
#include <vector>
#include <windows.h>

#include "platform/mn_thread.h"


namespace SL {
namespace Clipboard_Lite {
    struct Image;

    // HELPERS
    class ClipWrapper {
        HWND Hwnd;
        BOOL Opened;

      public:
        ClipWrapper(HWND h) : Hwnd(h), Opened(FALSE) { Opened = OpenClipboard(Hwnd); }
        operator bool() { return Opened == TRUE; }
        ~ClipWrapper()
        {
            if (Opened == TRUE) {
                CloseClipboard();
            }
        }
    };
    class GlobalLockWrapper {
      public:
        GlobalLockWrapper(void *ptr) : Ptr(ptr) {}
        ~GlobalLockWrapper()
        {
            if (Ptr != NULL) {
                GlobalUnlock(Ptr);
            }
        }
        operator bool() { return Ptr != NULL; }
        void *Ptr;
    };
    class HDCWrapper {
      public:
        HDCWrapper(HDC d) : DC(d) {}
        ~HDCWrapper()
        {
            if (DC != NULL) {
                DeleteDC(DC);
            }
        }
        operator bool() { return DC != NULL; }
        HDC DC;
    };
    class HBITMAPWrapper {
      public:
        HBITMAPWrapper(HBITMAP b) : Bitmap(b) {}
        ~HBITMAPWrapper()
        {
            if (Bitmap != NULL) {
                DeleteObject(Bitmap);
            }
        }
        operator bool() { return Bitmap != NULL; }
        HBITMAP Bitmap;
    };

    class Clipboard_Manager : public IClipboard_Manager
    {
      public:
        Clipboard_Manager();
        virtual ~Clipboard_Manager();

        virtual void run();
        virtual void copy(const std::string &test) override;
        virtual void copy(const Image &image) override;
        int thread_callback(int param);

        inline void RestoreClip(const std::string &buffer, UINT format)
        {
            if (OpenClipboard(Hwnd) == TRUE)
            {
                if (EmptyClipboard() == TRUE && buffer.size() > 0)
                {
                    HGDIOBJ hData = GlobalAlloc(GMEM_MOVEABLE, buffer.size()+1);
                    if (hData) {
                        LPTSTR pData = (LPTSTR)GlobalLock(hData);
                        if (pData) {
                            memcpy(pData, buffer.c_str(), buffer.size());
                            pData[buffer.size()]=0;
                            GlobalUnlock(hData);
                            if (::SetClipboardData(format, hData)) {
                                // clipboard takes ownership of the memory
                                hData = NULL;
                            }
                        }
                    }
                    if (hData) {
                        GlobalFree(hData);
                    }
                }
                CloseClipboard();
            }
        }


    private:
        void LoadClipImage();
        void LoadClipText();
    private:
        bool Copying;
        HWND Hwnd;
        MNThread* m_pThread;
    };
} // namespace Clipboard_Lite
} // namespace SL