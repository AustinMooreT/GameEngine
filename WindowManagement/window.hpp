#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <utility>

#ifdef VK_USE_PLATFORM_XCB_KHR
#include "xcb.hpp"
#endif

namespace SCREEN {

  class Window {
  private:
  public:
    struct Bindings {
#ifdef VK_USE_PLATFORM_XCB_KHR
      xcb_connection_t* connection;
      xcb_window_t window;
#endif
    };
    typedef std::size_t Width;
    typedef std::size_t Height;
    Window();
    void resize(const std::pair<Width, Height>& wh);
    std::pair<Width, Height> getDimensions() const;
    Bindings getBindings();
  };

  class Display {
  private:
  public:
    std::size_t getMonitorCount() const;

    //TODO add some notion of window settings.
    Window createWindow();
  };
};
#endif // WINDOW_HPP
