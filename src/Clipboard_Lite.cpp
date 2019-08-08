#include "Clipboard.h"
#include "Clipboard_Lite.h"
#include <assert.h>
//#include <functional>
#include <memory>

namespace SL {
namespace Clipboard_Lite {




    IClipboard_Manager* CreateClipboard()
    {
        SL::Clipboard_Lite::IClipboard_Manager* clipboard = new SL::Clipboard_Lite::Clipboard_Manager();
        return clipboard;
    }

} // namespace Clipboard_Lite
} // namespace SL
