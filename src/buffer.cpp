#include "buffer.hpp"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

std::vector<char> read_file(const std::filesystem::path &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }
  size_t file_size = static_cast<size_t>(file.tellg());
  std::vector<char> data;
  data.reserve(file_size);
  file.seekg(0);
  size_t counter = 0;
  while (counter++ != file_size)
    data.push_back(file.get());
  file.close();
  return data;
}

buffer_t::buffer_t() {
  // by default there is atleast 1 new line
  _data.push_back('\n');
  _bytes_per_line.push_back(1);
}
buffer_t::buffer_t(const std::filesystem::path &path) : _path(path) {
  if (std::filesystem::exists(_path)) {
    _data = read_file(_path);
    calculate_line_bytes();
  } else {
    // by default there is atleast 1 new line
    _data.push_back('\n');
    _bytes_per_line.push_back(1);
  }
}

void buffer_t::calculate_line_bytes() {
  size_t bytes = 0;
  for (const char ch : _data) {
    if (ch == '\n') {
      _bytes_per_line.push_back(bytes + 1);
      bytes = 0;
    } else {
      bytes++;
    }
  }
}

std::string buffer_t::get_line(size_t line_number) const {
  size_t line_start_byte_offset = 0;
  for (uint32_t i = 0; i < line_number; i++) {
    line_start_byte_offset += _bytes_per_line[i];
  }

  return std::string{
      &_data[line_start_byte_offset],
      &_data[line_start_byte_offset + _bytes_per_line[line_number] - 1]};
}

size_t buffer_t::get_num_lines() const { return _bytes_per_line.size(); }

void buffer_t::insert_new_line(size_t after_line_number) {
  size_t line_start_byte_offset = 0;
  for (uint32_t i = 0; i < after_line_number; i++) {
    line_start_byte_offset += _bytes_per_line[i];
  }
  line_start_byte_offset += _bytes_per_line[after_line_number];
  _data.insert(_data.begin() + line_start_byte_offset, '\n');
  _bytes_per_line.insert(_bytes_per_line.begin() + after_line_number + 1, 1);
}

void buffer_t::remove_line(size_t line_number) {
  size_t line_start_byte_offset = 0;
  for (uint32_t i = 0; i < line_number; i++) {
    line_start_byte_offset += _bytes_per_line[i];
  }
  size_t line_end_byte_offset =
      line_start_byte_offset + _bytes_per_line[line_number];
  if (line_number == _bytes_per_line.size() - 1) {
    _data.erase(_data.begin() + line_start_byte_offset, _data.end());
    _bytes_per_line.erase(_bytes_per_line.begin() + line_number);
  } else {
    std::memmove(&_data[line_start_byte_offset], &_data[line_end_byte_offset],
                 _data.size() - line_end_byte_offset);
    _data.resize(_data.size() - line_end_byte_offset + line_start_byte_offset);
    _bytes_per_line.erase(_bytes_per_line.begin() + line_number);
  }
}

void buffer_t::set_line(size_t line_number, const std::string &line) {
  // TODO: maybe use insert new line here rather than this ?
  if (line_number >= _bytes_per_line.size()) {
    while (line_number >= _bytes_per_line.size()) {
      _data.push_back('\n');
      _bytes_per_line.push_back(1);
    }
  }
  size_t line_start_byte_offset = 0;
  for (uint32_t i = 0; i < line_number; i++) {
    line_start_byte_offset += _bytes_per_line[i];
  }

  // TODO: assert line has no \n

  size_t line_end_byte_offset =
      line_start_byte_offset + _bytes_per_line[line_number];
  size_t line_size_in_bytes = line_end_byte_offset - line_start_byte_offset;
  size_t new_line_size_in_bytes = line.size() + 1; // +1 for \n
  int resize = new_line_size_in_bytes - line_size_in_bytes;
  size_t old_size = _data.size();

  if (resize > 0) {
    _data.resize(resize + old_size);
    if (line_end_byte_offset != old_size)
      std::memmove(&_data[line_end_byte_offset + resize],
                   &_data[line_end_byte_offset],
                   old_size - line_end_byte_offset);
    std::memmove(&_data[line_start_byte_offset], line.data(), line.size());
    _data[line_start_byte_offset + line.size()] = '\n';
    _bytes_per_line[line_number] = new_line_size_in_bytes;
  } else {
    if (line_end_byte_offset != old_size)
      std::memmove(&_data[line_end_byte_offset + resize],
                   &_data[line_end_byte_offset],
                   old_size - line_end_byte_offset);
    std::memmove(&_data[line_start_byte_offset], line.data(), line.size());
    _data.resize(resize + _data.size());
    _data[line_start_byte_offset + line.size()] = '\n';
    _bytes_per_line[line_number] = new_line_size_in_bytes;
  }
}

void buffer_t::save() {
  assert(_path != "");
  std::ofstream file{_path, std::ios::binary};
  if (!file.is_open())
    throw std::runtime_error("failed to open file for write: " +
                             _path.string());
  file.write(_data.data(), _data.size());
  if (!file)
    throw std::runtime_error("failed to write to file: " + _path.string());
}

void buffer_t::debug() {
  for (uint32_t i = 0; i < _data.size(); i++) {
    std::cout << std::hex << std::setfill('0') << std::setw(2)
              << (uint32_t)_data[i] << ' ';
    std::cout.flush();
    if (((i + 1) % 4) == 0)
      std::cout << "  ";
    if (((i + 1) % 8) == 0)
      std::cout << '\n';
  }
  std::cout << '\n';
}
