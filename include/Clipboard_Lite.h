#pragma once
#include <functional>
#include <memory>

#if defined(WINDOWS) || defined(WIN32)
#if defined(CLIPBOARD_LITE_DLL)
#define CLIPBOARD_LITE_EXTERN __declspec(dllexport)
#define CLIPBOARD_EXPIMP_TEMPLATE
#else
#define CLIPBOARD_LITE_EXTERN
#define CLIPBOARD_EXPIMP_TEMPLATE extern
#endif
#else
#define CLIPBOARD_LITE_EXTERN
#define CLIPBOARD_EXPIMP_TEMPLATE
#endif

namespace SL {
namespace Clipboard_Lite {
    // image data will be in r, g, b format   or r, g, b, a format
    struct Image {
        unsigned char* Data;
        int Height;
        int Width;
        int PixelStride;
        Image():Height(0),Width(0),PixelStride(0){}
    };

    class CLIPBOARD_LITE_EXTERN IClipboard_Configuration {
    public:
        virtual ~IClipboard_Configuration() {}
        virtual void onText(const std::string &text) = 0;
        virtual void onImage( const SL::Clipboard_Lite::Image &image) = 0;
    };

    class CLIPBOARD_LITE_EXTERN IClipboard_Manager
    {
      public:
        virtual ~IClipboard_Manager() {}

        virtual void setConfiguration(IClipboard_Configuration* pConfiguration){m_pConfiguration = pConfiguration;}
        // copy text into the clipboard
        virtual void copy(const std::string &text) = 0;
        // copy an image into the clipboard... image data will be in r, g, b format   or r, g, b, a format
        virtual void copy(const Image &img) = 0;
        virtual void run() = 0;
    protected:
        IClipboard_Configuration* m_pConfiguration;
    };

    CLIPBOARD_LITE_EXTERN IClipboard_Manager* CreateClipboard();

} // namespace Clipboard_Lite
} // namespace SL
