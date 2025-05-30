#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <filesystem>
#include <vector>

struct buffer_t {
  buffer_t();
  buffer_t(const std::filesystem::path &path);

  std::string get_line(size_t line_number);
  size_t get_num_lines();
  void set_line(size_t line_number, const std::string &line);

  void debug();

private:
  void calculate_line_bytes();

  std::filesystem::path _path;
  std::vector<char> _data;
  std::vector<size_t> _bytes_per_line;
};

#endif // !BUFFER_HPP
