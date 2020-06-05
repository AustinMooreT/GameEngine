#ifndef XCB_RAW_HPP
#define XCB_RAW_HPP

#include <xcb/xcb.h>
#include <iostream>

namespace xcbraw {

  static inline xcb_intern_atom_reply_t* intern_atom_helper(xcb_connection_t* connection,
                                                            bool onlyIfExists, const std::string& str) {
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection,
                                                      onlyIfExists,
                                                      str.size(), str.c_str());
    return xcb_intern_atom_reply(connection, cookie, NULL);
  }

  xcb_connection_t* createXcbConnection(int* screen) {
    xcb_connection_t* connection = xcb_connect(NULL, screen);
    if (connection == NULL) {
      std::cerr << "xcb::initXcbConnection::connection_IS_NULL\n";
    }
    return connection;
  }

  xcb_screen_t* createXcbScreen(xcb_connection_t* connection,
                                int* scr) {
    xcb_screen_t* screen;
    const xcb_setup_t* setup;
    xcb_screen_iterator_t iter;
    //NOTE idk how the rest of this works.
    //It was stolen from the internet.
    setup = xcb_get_setup(connection);
    iter = xcb_setup_roots_iterator(setup);
    while ((*scr)-- > 0) {
      xcb_screen_next(&iter);
    }
    screen = iter.data;
    return screen;
  }

  xcb_window_t createXcbWindow(xcb_connection_t* connection,
                               xcb_screen_t* screen,
                               const std::string& title,
                               bool fullscreen,
                               uint32_t width, uint32_t height) {
    xcb_window_t window = xcb_generate_id(connection);
    uint32_t value_mask, value_list[32];

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = screen->black_pixel;
    value_list[1] =
      XCB_EVENT_MASK_KEY_RELEASE |
      XCB_EVENT_MASK_KEY_PRESS |
      XCB_EVENT_MASK_EXPOSURE |
      XCB_EVENT_MASK_STRUCTURE_NOTIFY |
      XCB_EVENT_MASK_POINTER_MOTION |
      XCB_EVENT_MASK_BUTTON_PRESS |
      XCB_EVENT_MASK_BUTTON_RELEASE;

    xcb_create_window(connection,
                      XCB_COPY_FROM_PARENT,
                      window, screen->root,
                      0, 0, width, height, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual,
                      value_mask, value_list);

    auto atom_wm_delete_window = intern_atom_helper(connection, false, "WM_DELETE_WINDOW");

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_reply_t* reply = intern_atom_helper(connection, true, "WM_PROTOCOLS");
    atom_wm_delete_window = intern_atom_helper(connection, false, "WM_DELETE_WINDOW");

    xcb_change_property(connection, XCB_PROP_MODE_REPLACE,
                        window, (*reply).atom, 4, 32, 1,
                        &(*atom_wm_delete_window).atom);

    xcb_change_property(connection, XCB_PROP_MODE_REPLACE,
                        window, (*reply).atom, 4, 32, 1,
                        &(*atom_wm_delete_window).atom);

    xcb_change_property(connection, XCB_PROP_MODE_REPLACE,
                        window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
                        title.size(), title.c_str());

    free(reply);

    if(fullscreen) {
      xcb_intern_atom_reply_t *atom_wm_state = intern_atom_helper(connection, false, "_NET_WM_STATE");
      xcb_intern_atom_reply_t *atom_wm_fullscreen = intern_atom_helper(connection, false, "_NET_WM_STATE_FULLSCREEN");
      xcb_change_property(connection,
                          XCB_PROP_MODE_REPLACE,
                          window, atom_wm_state->atom,
                          XCB_ATOM_ATOM, 32, 1,
                          &(atom_wm_fullscreen->atom));
      free(atom_wm_fullscreen);
      free(atom_wm_state);
    }
    xcb_map_window(connection, window);
    return window;
  }
};

#endif // XCB_RAW_HPP
