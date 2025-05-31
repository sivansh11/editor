#include <cassert>
#include <cctype>
#include <fstream>
#include <iostream>
#include <ncurses.h>

#include "buffer.hpp"

std::ofstream log{"/tmp/log"};

#define ctrl(x) ((x) & 0x1f)

struct editor_t {
  editor_t(int height, int width, int y, int x)
      : _height(height), _width(width), _y(y), _x(x) {
    _main_window = newwin(_height - 1, _width, _y, _x);
    _debug = newwin(1, _width, _y + _height - 1, _x);
    keypad(_main_window, TRUE);
  }
  ~editor_t() { delwin(_main_window); }

  void cursor_down(const buffer_t &buffer) {
    if (_cursor_y < static_cast<int>(buffer.get_num_lines() - 1)) {
      _cursor_y++;
      if (_cursor_x > static_cast<int>(buffer.get_line(_cursor_y).size()))
        _cursor_x = static_cast<int>(buffer.get_line(_cursor_y).size());
    }
  }
  void cursor_up(const buffer_t &buffer) {
    if (_cursor_y > 0) {
      _cursor_y--;
      if (_cursor_x > static_cast<int>(buffer.get_line(_cursor_y).size()))
        _cursor_x = static_cast<int>(buffer.get_line(_cursor_y).size());
    }
  }
  void cursor_left(const buffer_t &buffer) {
    if (_cursor_x > 0)
      _cursor_x--;
  }
  void cursor_right(const buffer_t &buffer) {
    if (_cursor_x < static_cast<int>(buffer.get_line(_cursor_y).size()))
      _cursor_x++;
  }
  void insert_ch(buffer_t &buffer, int ch) {
    assert(std::isprint(ch));
    std::string str = buffer.get_line(_cursor_y);
    char character = ch;
    str.insert(_cursor_x, 1, character);
    buffer.set_line(_cursor_y, str);
    _cursor_x++;
  }
  void insert_new_line(buffer_t &buffer) {
    buffer.insert_new_line(_cursor_y);
    int old_cursor_x = _cursor_x;
    int old_cursor_y = _cursor_y;
    _cursor_x = 0;
    _cursor_y++;
    std::string str = buffer.get_line(old_cursor_y);
    std::string sub_str = str.substr(old_cursor_x, str.size());
    str.erase(old_cursor_x, str.size());
    buffer.set_line(old_cursor_y, str);
    buffer.set_line(_cursor_y, sub_str);
  }
  void remove_ch(buffer_t &buffer) {
    std::string str = buffer.get_line(_cursor_y);
    if (str == "") {
      if (_cursor_y != 0) {
        buffer.remove_line(_cursor_y);
        _cursor_y--;
        _cursor_x = static_cast<int>(buffer.get_line(_cursor_y).size());
      }
    } else {
      if (_cursor_x != 0) {
        str.erase(_cursor_x-- - 1, 1);
        buffer.set_line(_cursor_y, str);
      } else {
        if (_cursor_y != 0) {
          std::string aboveline = buffer.get_line(_cursor_y - 1);
          buffer.remove_line(_cursor_y);
          _cursor_y--;
          _cursor_x = static_cast<int>(buffer.get_line(_cursor_y).size());
          buffer.set_line(_cursor_y, aboveline + str);
        }
      }
    }
  }

  void draw_buffer(buffer_t &buffer) {
    werase(_main_window);
    for (uint32_t i = 0; i < buffer.get_num_lines(); i++) {
      std::string line = buffer.get_line(i);
      mvwprintw(_main_window, i, 0, "%s", line.c_str());
    }
    mvwprintw(_debug, 0, 0, "%i %i", _cursor_y, _cursor_x);
    wrefresh(_debug);
    wmove(_main_window, _cursor_y, _cursor_x);
    wrefresh(_main_window);
  }

  void handle_input(int ch, buffer_t &buffer) {
    switch (ch) {
    case KEY_DOWN:
      cursor_down(buffer);
      break;
    case KEY_UP:
      cursor_up(buffer);
      break;
    case KEY_RIGHT:
      cursor_right(buffer);
      break;
    case KEY_LEFT:
      cursor_left(buffer);
      break;
    case '\n':
      insert_new_line(buffer);
      break;
    case KEY_BACKSPACE:
      remove_ch(buffer);
      break;
    case ctrl('s'):
      buffer.save();
      break;
    default:
      if (std::isprint(ch))
        insert_ch(buffer, ch);
      else {
        log << "UNKNOWN KEY: " << ch << '\n';
        log.flush();
      }
      break;
    }
  }

  WINDOW *_main_window;
  WINDOW *_debug;
  int _height, _width, _y, _x;
  int _cursor_y = 0, _cursor_x = 0;
};

enum selected_window_t {
  e_none,
  e_editor,
};

int main(int argc, char **argv) {
  // buffer_t buffer{argv[1]};
  // for (uint32_t i = 0; i < buffer.get_num_lines(); i++) {
  //   std::cout << buffer.get_line(i) << '\n';
  // }
  // std::cout << "**********************\n";
  // buffer.remove_line(1);
  // for (uint32_t i = 0; i < buffer.get_num_lines(); i++) {
  //   std::cout << buffer.get_line(i) << '\n';
  // }

  initscr();
  raw();
  noecho();
  set_escdelay(0);
  keypad(stdscr, TRUE);
  refresh();

  // editor state
  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);
  editor_t editor{max_y, max_x, 0, 0};
  selected_window_t selected_window = selected_window_t::e_editor;

  buffer_t buffer;
  if (argc == 2)
    buffer = buffer_t{argv[1]};

  bool running = true;
  while (running) {
    editor.draw_buffer(buffer);

    int ch = getch();

    switch (ch) {
    case 27:
      running = false;
      break;
    default:
      switch (selected_window) {
      case e_editor:
        editor.handle_input(ch, buffer);
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
