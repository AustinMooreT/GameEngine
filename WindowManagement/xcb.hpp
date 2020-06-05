#ifndef XCB_HPP
#define XCB_HPP

#include "xcbraw.hpp"

namespace xcb {
  xcb_connection_t* connection;
  xcb_screen_t* screen;
  xcb_window_t window;

  void initXcb(bool fullscreen, int width, int height, const std::string& title) {
    int screenInt;
    connection = xcbraw::createXcbConnection(&screenInt);
    screen = xcbraw::createXcbScreen(connection, &screenInt);
    window = xcbraw::createXcbWindow(connection, screen, title, fullscreen, width, height);
  }

  void cleanupXcb() {
    xcb_disconnect(connection);
  }


}


#endif // XCB_HPP
