#include <ncurses.h>

#include "buffer.hpp"

struct editor_t {
  editor_t(int height, int width, int y, int x)
      : _height(height), _width(width), _y(y), _x(x) {
    _window = newwin(_height, _width, _y, _x);
    keypad(_window, TRUE);
  }
  ~editor_t() { delwin(_window); }

  void cursor_down() {
    if (_cursor_y < _height - 1)
      _cursor_y++;
  }
  void cursor_up() {
    if (_cursor_y > 0)
      _cursor_y--;
  }
  void cursor_left() {
    if (_cursor_x > 0)
      _cursor_x--;
  }
  void cursor_right() {
    if (_cursor_x < _width - 1)
      _cursor_x++;
  }

  void draw_buffer(buffer_t &buffer) {
    for (uint32_t i = 0; i < buffer.get_num_lines(); i++) {
      std::string line = buffer.get_line(i);
      mvwprintw(_window, i, 0, "%s", line.data());
    }
    wrefresh(_window);
    wmove(_window, _cursor_y, _cursor_x);
  }

  void handle_input(int ch) {
    switch (ch) {
    case KEY_DOWN:
      cursor_down();
      break;
    case KEY_UP:
      cursor_up();
      break;
    case KEY_RIGHT:
      cursor_right();
      break;
    case KEY_LEFT:
      cursor_left();
      break;
    }
  }

  WINDOW *_window;
  int _height, _width, _y, _x;
  int _cursor_y = 0, _cursor_x = 0;
};

enum selected_window_t {
  e_none,
  e_editor,
};

int main(int argc, char **argv) {
  initscr();
  raw();
  noecho();
  set_escdelay(0);
  keypad(stdscr, TRUE);
  refresh();

  // editor state
  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);
  editor_t editor{max_y, max_x - 4, 0, 4};
  selected_window_t selected_window = selected_window_t::e_editor;

  buffer_t buffer;
  if (argc == 2)
    buffer = buffer_t{argv[1]};

  bool running = true;
  while (running) {
    editor.draw_buffer(buffer);

    int ch;

    switch (selected_window) {
    case e_editor:
      ch = wgetch(editor._window);
      break;
    case e_none:
      ch = getch();
      break;
    }
    switch (ch) {
    case 27:
      running = false;
      break;
    default:
      switch (selected_window) {
      case e_editor:
        editor.handle_input(ch);
        break;
      case e_none:
        break;
      }
      break;
    }
  }

  endwin();
  return 0;
}
