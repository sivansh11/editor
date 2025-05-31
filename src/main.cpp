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
    _input = newwin(_height - 1, _width, _y, _x);
    _status = newwin(1, _width, _y + _height - 1, _x);
    keypad(_input, TRUE);

    werase(_status);
    mvwprintw(_status, 0, _width - 20, "%i %i", _cursor_y, _cursor_x);
    wrefresh(_status);
    werase(_input);
    wrefresh(_input);
  }
  ~editor_t() {
    delwin(_input);
    delwin(_status);
  }

  void key_down(const buffer_t &buffer) {
    if (_cursor_y < static_cast<int>(buffer.get_num_lines() - 1)) {
      _cursor_y++;
      if (_cursor_x > static_cast<int>(buffer.get_line(_cursor_y).size()))
        _cursor_x = static_cast<int>(buffer.get_line(_cursor_y).size());
      update_input_window(buffer);
    }
  }
  void key_up(const buffer_t &buffer) {
    if (_cursor_y > 0) {
      _cursor_y--;
      if (_cursor_x > static_cast<int>(buffer.get_line(_cursor_y).size()))
        _cursor_x = static_cast<int>(buffer.get_line(_cursor_y).size());
      update_input_window(buffer);
    }
  }
  void key_left(const buffer_t &buffer) {
    if (_cursor_x > 0) {
      _cursor_x--;
      update_input_window(buffer);
    }
  }
  void key_right(const buffer_t &buffer) {
    if (_cursor_x < static_cast<int>(buffer.get_line(_cursor_y).size())) {
      _cursor_x++;
      update_input_window(buffer);
    }
  }
  void insert_ch(buffer_t &buffer, int ch) {
    assert(std::isprint(ch));
    char character = ch;
    switch (_mode) {
    case e_editor: {
      std::string str = buffer.get_line(_cursor_y);
      str.insert(_cursor_x, 1, character);
      buffer.set_line(_cursor_y, str);
      _cursor_x++;
      update_input_window(buffer);
    } break;
    case e_command:
      _command += character;
      update_status_window(buffer);
      break;
    }
  }
  void key_enter(buffer_t &buffer) {
    switch (_mode) {
    case e_editor: {
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
      update_input_window(buffer);
    } break;
    case e_command:
      // TODO: handle commands
      _command = "";
      update_status_window(buffer);
      break;
    }
  }
  void key_backspace(buffer_t &buffer) {
    switch (_mode) {
    case e_editor: {
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
      update_input_window(buffer);
    } break;
    case e_command:
      if (_command.size()) {
        _command.pop_back();
        update_status_window(buffer);
      }
      break;
    }
  }
  void key_esc(buffer_t &buffer) {
    switch (_mode) {
    case e_editor:
      break;
    case e_command:
      _mode = e_editor;
      _command = "";
      update_status_window(buffer);
      update_input_window(buffer);
      break;
    }
  }

  void update_input_window(const buffer_t &buffer) {
    werase(_input);
    for (uint32_t i = 0; i < buffer.get_num_lines(); i++) {
      std::string line = buffer.get_line(i);
      mvwprintw(_input, i, 0, "%s", line.c_str());
    }
    wrefresh(_input);
  }
  void update_status_window(const buffer_t &buffer) {
    werase(_status);
    mvwprintw(_status, 0, _width - 20, "%i %i", _cursor_y, _cursor_x);
    mvwprintw(_status, 0, 0, "%s", _command.c_str());
    wrefresh(_status);
  }

  // TODO: figure out smart updates
  // void draw_buffer(buffer_t &buffer) {
  //   werase(_input);
  //   werase(_status);
  //   for (uint32_t i = 0; i < buffer.get_num_lines(); i++) {
  //     std::string line = buffer.get_line(i);
  //     mvwprintw(_input, i, 0, "%s", line.c_str());
  //   }
  //   mvwprintw(_status, 0, _width - 20, "%i %i", _cursor_y, _cursor_x);
  //   mvwprintw(_status, 0, 0, "%s", _command.c_str());
  //   switch (_mode) {
  //   case e_editor:
  //     wmove(_input, _cursor_y, _cursor_x);
  //     wrefresh(_status);
  //     wrefresh(_input);
  //     break;
  //   case e_command:
  //     wmove(_status, 0, _command.size());
  //     wrefresh(_input);
  //     wrefresh(_status);
  //     break;
  //   }
  // }

  void handle_input(int ch, buffer_t &buffer) {
    switch (ch) {
    case KEY_DOWN:
      key_down(buffer);
      break;
    case KEY_UP:
      key_up(buffer);
      break;
    case KEY_RIGHT:
      key_right(buffer);
      break;
    case KEY_LEFT:
      key_left(buffer);
      break;
    case '\n':
      key_enter(buffer);
      break;
    case KEY_BACKSPACE:
      key_backspace(buffer);
      break;
    case 27:
      key_esc(buffer);
      break;
    case ctrl('s'):
      buffer.save();
      break;
    case ctrl('p'):
      _mode = e_command;
      _command = "";
      update_status_window(buffer);
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

  enum mode_t {
    e_editor,
    e_command,
  };

  WINDOW *_input;
  WINDOW *_status;
  mode_t _mode = e_editor;
  std::string _command;
  int _height, _width, _y, _x;
  int _cursor_y = 0, _cursor_x = 0;
};

// enum selected_window_t {
//   e_none,
//   e_editor,
// };

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
  // selected_window_t selected_window = selected_window_t::e_editor;

  buffer_t buffer;
  if (argc == 2)
    buffer = buffer_t{argv[1]};

  bool running = true;
  while (running) {
    // editor.draw_buffer(buffer);

    int ch = getch();

    switch (ch) {
    case ctrl('c'):
      running = false;
      break;
    default:
      editor.handle_input(ch, buffer);
      // switch (selected_window) {
      // case e_editor:
      //   editor.handle_input(ch, buffer);
      //   break;
      // case e_none:
      //   break;
      // }
      break;
    }
  }

  endwin();
  return 0;
}
