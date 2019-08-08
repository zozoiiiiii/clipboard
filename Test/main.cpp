#include "Clipboard_Lite.h"
//#include <chrono>
#include <iostream>
#include <string>
//#include <thread>
#include <Windows.h>

// THESE LIBRARIES ARE HERE FOR CONVINIENCE!! They are SLOW and ONLY USED FOR
// HOW THE LIBRARY WORKS!
#define TJE_IMPLEMENTATION
#include "tiny_jpeg.h"
#define LODEPNG_COMPILE_PNG
#define LODEPNG_COMPILE_DISK
#include "lodepng.h"
/////////////////////////////////////////////////////////////////////////

//using namespace std::chrono_literals;



class Clipboard_Configuration : public SL::Clipboard_Lite::IClipboard_Configuration {

public:
    virtual ~Clipboard_Configuration() {}
    virtual void onText(const std::string &text) override
    {
        std::cout << "onText" << std::endl;
        std::cout << text << std::endl;
    }

    virtual void onImage(const SL::Clipboard_Lite::Image &image) override
    {
        //         if (image.PixelStride == 4)
        //         {
        //           s += std::string(".png");
        //           lodepng::encode(s.c_str(), image.Data.get(), image.Width, image.Height);
        //         }
        //         else if (image.PixelStride == 3)
        //         {
        //           s += std::string(".jpg");
        //           tje_encode_to_file(s.c_str(), image.Width, image.Height, 3, image.Data.get());
        //         }
        std::cout << "onImage Height=" << image.Height << " Width=" << image.Width << std::endl;
    }
};


int main(int argc, char *argv[])
{

    int counter = 0;
    std::cout << "Clipboard Example running" << std::endl;

    SL::Clipboard_Lite::IClipboard_Configuration* pConfiguration =   new Clipboard_Configuration();
   SL::Clipboard_Lite::IClipboard_Manager* clipboard  = SL::Clipboard_Lite::CreateClipboard();
   clipboard->setConfiguration(pConfiguration);
    clipboard->run();

    std::cout << "Copying Text to clipboard" << std::endl;
    std::string txt = "pasted text";
    clipboard->copy(txt);

    int i = 0;
    while (i++ < 1000) {

        txt = "pasted text a time ";

        char buf[10];
        itoa(i, buf, 10);
        txt += buf;
        clipboard->copy(txt);
        //std::this_thread::sleep_for(10s);

        std::cout << txt << std::endl;
        Sleep(1000);
    }

    return 0;
}
