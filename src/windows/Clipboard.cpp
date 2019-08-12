#include "Clipboard.h"
#include "Clipboard_Lite.h"
#include <assert.h>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace SL {
namespace Clipboard_Lite {
    Clipboard_Manager::Clipboard_Manager() { Copying = false; }
    Clipboard_Manager::~Clipboard_Manager()
    {
        if (Hwnd) {
            PostMessage(Hwnd, WM_QUIT, 0, 0);
        }
        if (m_pThread)
        {
            delete m_pThread;
            m_pThread = NULL;
        }
    }

    void Clipboard_Manager::LoadClipImage()
    {
        bool c = ClipWrapper(Hwnd);
        if (!c)
            return;

        if (IsClipboardFormatAvailable(CF_BITMAP) || IsClipboardFormatAvailable(CF_DIB) || IsClipboardFormatAvailable(CF_DIBV5))
        {
            HANDLE hClipboard = (BITMAPINFO *)GetClipboardData(CF_DIB);
            if (hClipboard) {
                GlobalLockWrapper dib(GlobalLock(hClipboard));
                if (dib) {
                    LPBITMAPINFO info = reinterpret_cast<LPBITMAPINFO>(dib.Ptr);
                    Image img;
                    img.Width = info->bmiHeader.biWidth;
                    img.Height = info->bmiHeader.biHeight;
                    img.PixelStride = info->bmiHeader.biBitCount / 8;

                    if ((info->bmiHeader.biBitCount == 24 || info->bmiHeader.biBitCount == 32) && info->bmiHeader.biCompression == BI_RGB &&
                        info->bmiHeader.biClrUsed == 0) {

                        img.Data = new unsigned char[info->bmiHeader.biSizeImage];
                        memcpy(img.Data, (info + 1), info->bmiHeader.biSizeImage);

                        int linewidth = 0;
                        int depth = info->bmiHeader.biBitCount / 8;
                        if (depth == 3)
                            linewidth = 4 * ((3 * img.Width + 3) / 4);
                        else
                            linewidth = 4 * img.Width;

                        unsigned char*p = img.Data;

                        for (int i = img.Height - 1; i >= 0; i--)
                        {
                            unsigned char* r = img.Data + (img.Width * img.PixelStride * i);
                            for (int j = 0; j < img.Width; j++)
                            {
                                int bb = *r++;
                                int gg = *r++;
                                int rr = *r++;
                                *p++ = rr;
                                *p++ = gg;
                                *p++ = bb;
                                if (img.PixelStride == 4)
                                    *p++ = *r++;
                            }
                        }
                    }
                    else {
                        HDCWrapper dc(GetDC(NULL));
                        HDCWrapper capturedc(CreateCompatibleDC(dc.DC));
                        HBITMAPWrapper bitmap(CreateCompatibleBitmap(dc.DC, img.Width, img.Height));

                        HGDIOBJ originalBmp = SelectObject(capturedc.DC, bitmap.Bitmap);

                        void *pDIBBits = (void *)(info->bmiColors);
                        if (info->bmiHeader.biCompression == BI_BITFIELDS)
                            pDIBBits = (void *)(info->bmiColors + 3);
                        else if (info->bmiHeader.biClrUsed > 0)
                            pDIBBits = (void *)(info->bmiColors + info->bmiHeader.biClrUsed);

                        SetDIBitsToDevice(capturedc.DC, 0, 0, img.Width, img.Height, 0, 0, 0, img.Height, pDIBBits, info, DIB_RGB_COLORS);

                        BITMAPINFOHEADER bi;
                        memset(&bi, 0, sizeof(bi));

                        bi.biSize = sizeof(BITMAPINFOHEADER);
                        bi.biWidth = img.Width;
                        bi.biHeight = -img.Height;
                        bi.biPlanes = 1;
                        bi.biBitCount = static_cast<WORD>(img.PixelStride * 8);
                        bi.biCompression = BI_RGB;
                        bi.biSizeImage = ((img.Width * bi.biBitCount + 31) / (img.PixelStride * 8)) * img.PixelStride * img.Height;
                        img.Data = new unsigned char[bi.biSizeImage];

                        GetDIBits(capturedc.DC, bitmap.Bitmap, 0, (UINT)img.Height, img.Data, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

                        SelectObject(capturedc.DC, originalBmp);

                        unsigned char*p = img.Data;

                        for (int i = 0; i < img.Height; i++) {
                            unsigned char* r = img.Data + (img.Width * img.PixelStride * i);
                            for (int j = 0; j < img.Width; j++) {
                                int bb = *r++;
                                int gg = *r++;
                                int rr = *r++;
                                *p++ = rr; // we want RGB
                                *p++ = gg;
                                *p++ = bb;
                                if (img.PixelStride == 4)
                                    *p++ = *r++;
                            }
                        }
                    }
                    m_pConfiguration->onImage(img);
                }
            }
        }
    }
    void Clipboard_Manager::LoadClipText()
    {
        if (!IsClipboardFormatAvailable(CF_TEXT))
            return;

        //bool c = ClipWrapper(Hwnd);
        //if (!c)
          //  return;


        if (!OpenClipboard(Hwnd)) 
            return; 
        HGLOBAL h = ::GetClipboardData(CF_TEXT);
        if (!h)
        {
                return;
        }

        LPVOID pData = GlobalLock(h);
        SIZE_T nLength = GlobalSize(h);
        if (pData && nLength > 0)
        {
            std::string buffer;
            buffer.resize(nLength);
            memcpy((void *)buffer.data(), pData, nLength);
            GlobalUnlock(h);
            m_pConfiguration->onText(buffer);
        }
    }


    void ThreadFunc(void* lpParameter)
    {
        Clipboard_Manager* pManager = (Clipboard_Manager*)lpParameter;
        pManager->thread_callback(0);
    }

    void Clipboard_Manager::run()
    {
        m_pThread = new MNThread(&ThreadFunc, this);
    }


    int Clipboard_Manager::thread_callback(int param)
    {
            WNDCLASSEX wndclass = {};
            memset(&wndclass, 0, sizeof(wndclass));
            wndclass.cbSize = sizeof(WNDCLASSEX);
            wndclass.lpfnWndProc = DefWindowProc;
            wndclass.lpszClassName = _T("myclass");
            if (!RegisterClassEx(&wndclass))
                return 0;

            Hwnd = CreateWindowEx(0, wndclass.lpszClassName, _T("clipwatcher"), 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0);
            if (!Hwnd) 
            {
                UnregisterClass(wndclass.lpszClassName, NULL);
                return 0;
            }

            if (AddClipboardFormatListener(Hwnd))
            {
                MSG msg;
                while (GetMessage(&msg, Hwnd, 0, 0) != 0)
                {
                    if (msg.message == WM_CLIPBOARDUPDATE)
                    {
                        //if (!Copying) {
                            if (m_pConfiguration) {
                                LoadClipText();
                                LoadClipImage();
                            }
                        //}
                        Copying = false;
                    }
                    else {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
                RemoveClipboardFormatListener(Hwnd);
            }
            UnregisterClass(wndclass.lpszClassName, NULL);
            return 1;
    }

    void Clipboard_Manager::copy(const std::string &text)
    {
        Copying = true;
        RestoreClip(text, CF_TEXT);
    }

    void Clipboard_Manager::copy(const Image &image)
    {
        Copying = true;
        BITMAPINFOHEADER bmih;
        memset(&bmih, 0, sizeof(BITMAPINFOHEADER));

        bmih.biWidth = image.Width;
        bmih.biHeight = -image.Height;
        bmih.biBitCount = image.PixelStride * 8;
        bmih.biCompression = BI_RGB;
        bmih.biSize = sizeof(BITMAPINFOHEADER);
        bmih.biPlanes = 1;

        BITMAPINFO dbmi;
        ZeroMemory(&dbmi, sizeof(dbmi));
        dbmi.bmiHeader = bmih;
        dbmi.bmiColors->rgbBlue = 0;
        dbmi.bmiColors->rgbGreen = 0;
        dbmi.bmiColors->rgbRed = 0;
        dbmi.bmiColors->rgbReserved = 0;

        HDC hdc = ::GetDC(NULL);
        CreateDIBitmap(hdc, &bmih, CBM_INIT, image.Data, &dbmi, DIB_RGB_COLORS);
        ::ReleaseDC(NULL, hdc);

        // HANDLE hData = GlobalAlloc(GHND | GMEM_SHARE,  sizeof(BITMAPINFO) + m_bmi.bmiHeader.biWidth*m_bmi.bmiHeader.biHeight * 3);
        // LPVOID pData = (LPVOID)GlobalLock(hData);
        // memcpy(pData, &m_bmi, sizeof(BITMAPINFO));
        // memcpy((UCHAR *)pData + sizeof(BITMAPINFO), (LPVOID)m_pUTDispData, m_bmi.bmiHeader.biWidth*m_bmi.bmiHeader.biHeight * 3);
        // GlobalUnlock(hData);

        //// cleanup
        // DeleteObject(hbmp);
        // if (OpenClipboard(Hwnd) == TRUE) {

        //    if (EmptyClipboard() == TRUE && (image.Height > 0 && image.Width >0)) {
        //        auto hData = GlobalAlloc(GMEM_MOVEABLE, buffer.size());
        //        if (hData) {
        //            auto pData = GlobalLock(hData);
        //            if (pData) {
        //                memcpy(pData, buffer.data(), buffer.size());
        //                GlobalUnlock(hData);
        //                if (::SetClipboardData(CF_BITMAP, hData)) {
        //                    //clipboard takes ownership of the memory
        //                    hData = NULL;
        //                }
        //            }
        //        }
        //        if (hData) {
        //            GlobalFree(hData);
        //        }
        //    }
        //    CloseClipboard();

        //}
    }
}; // namespace Clipboard_Lite
} // namespace SL