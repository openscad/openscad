#pragma once

#include <iterator>
#include <utility>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>

#include <glib.h>

class str_utf8_wrapper
{
private:
  // store the cached length in glong, paired with its string
  struct str_utf8_t {
    static constexpr size_t LENGTH_UNKNOWN = -1;
    str_utf8_t() : u8str(), u8len(0) {
    }
    str_utf8_t(std::string s) : u8str(std::move(s)) {
    }
    str_utf8_t(const char *cstr) : u8str(cstr) {
    }
    str_utf8_t(const char *cstr, size_t size, size_t u8len) : u8str(cstr, size), u8len(u8len) {
    }
    const std::string u8str;
    size_t u8len = LENGTH_UNKNOWN;
  };
  // private constructor for copying members
  explicit str_utf8_wrapper(const std::shared_ptr<str_utf8_t>& str_in) : str_ptr(str_in) { }

public:
  class iterator
  {
public:
    // iterator_traits required types:
    using iterator_category = std::forward_iterator_tag;
    using value_type = str_utf8_wrapper;
    using difference_type = void;
    using reference = value_type; // type used by operator*(), not actually a reference
    using pointer = void;
    iterator() : ptr(&nullterm) {} // DefaultConstructible
    iterator(const str_utf8_wrapper& str) : ptr(str.c_str()), len(char_len()) { }
    iterator(const str_utf8_wrapper& str, bool /*end*/) : ptr(str.c_str() + str.size()) { }

    iterator& operator++() { ptr += len; len = char_len(); return *this; }
    reference operator*() { return {ptr, len}; } // Note: returns a new str_utf8_wrapper **by value**, representing a single UTF8 character.
    bool operator==(const iterator& other) const { return ptr == other.ptr; }
    bool operator!=(const iterator& other) const { return ptr != other.ptr; }
private:
    size_t char_len() { return g_utf8_next_char(ptr) - ptr; };
    static const char nullterm = '\0';
    const char *ptr;
    size_t len = 0;
  };

  [[nodiscard]] iterator begin() const { return {*this}; }
  [[nodiscard]] iterator end() const { return {*this, true}; }
  str_utf8_wrapper() : str_ptr(std::make_shared<str_utf8_t>()) { }
  str_utf8_wrapper(const std::string& s) : str_ptr(std::make_shared<str_utf8_t>(s)) { }
  str_utf8_wrapper(const char *cstr) : str_ptr(std::make_shared<str_utf8_t>(cstr)) { }
  // for enumerating single utf8 chars from iterator
  str_utf8_wrapper(const char *cstr, size_t clen) : str_ptr(std::make_shared<str_utf8_t>(cstr, clen, 1)) { }
  str_utf8_wrapper(uint32_t unicode) {
    char out[6] = " ";
    if (unicode != 0 && g_unichar_validate(unicode)) {
        g_unichar_to_utf8(unicode, out);
    }
    str_ptr = std::make_shared<str_utf8_t>(out);
  }
  str_utf8_wrapper(const str_utf8_wrapper&) = delete; // never copy, move instead
  str_utf8_wrapper& operator=(const str_utf8_wrapper&) = delete; // never copy, move instead
  str_utf8_wrapper(str_utf8_wrapper&&) = default;
  str_utf8_wrapper& operator=(str_utf8_wrapper&&) = default;
  ~str_utf8_wrapper() = default;
  [[nodiscard]] str_utf8_wrapper clone() const { return str_utf8_wrapper(this->str_ptr); } // makes a copy of shared_ptr

  bool operator==(const str_utf8_wrapper& rhs) const { return this->str_ptr->u8str == rhs.str_ptr->u8str; }
  bool operator!=(const str_utf8_wrapper& rhs) const { return this->str_ptr->u8str != rhs.str_ptr->u8str; }
  bool operator<(const str_utf8_wrapper& rhs) const { return this->str_ptr->u8str < rhs.str_ptr->u8str; }
  bool operator>(const str_utf8_wrapper& rhs) const { return this->str_ptr->u8str > rhs.str_ptr->u8str; }
  bool operator<=(const str_utf8_wrapper& rhs) const { return this->str_ptr->u8str <= rhs.str_ptr->u8str; }
  bool operator>=(const str_utf8_wrapper& rhs) const { return this->str_ptr->u8str >= rhs.str_ptr->u8str; }
  [[nodiscard]] bool empty() const { return this->str_ptr->u8str.empty(); }
  [[nodiscard]] const char *c_str() const { return this->str_ptr->u8str.c_str(); }
  [[nodiscard]] const std::string& toString() const { return this->str_ptr->u8str; }
  [[nodiscard]] size_t size() const { return this->str_ptr->u8str.size(); }
  str_utf8_wrapper operator[](const size_t idx) const {
    if (idx < this->size()) {
      // Ensure character (not byte) index is inside the character/glyph array
      if (idx < this->get_utf8_strlen()) {
        gchar utf8_of_cp[6] = ""; //A buffer for a single unicode character to be copied into
	auto ptr = g_utf8_offset_to_pointer(str_ptr->u8str.c_str(), idx);
	if (ptr) {
          g_utf8_strncpy(utf8_of_cp, ptr, 1);
        }
        return std::string(utf8_of_cp);
      }
    }
    return {};
  }

  [[nodiscard]] size_t get_utf8_strlen() const {
    if (str_ptr->u8len == str_utf8_t::LENGTH_UNKNOWN) {
      str_ptr->u8len = g_utf8_strlen(str_ptr->u8str.c_str(), static_cast<gssize>(str_ptr->u8str.size()));
    }
    return str_ptr->u8len;
  }

  [[nodiscard]] uint32_t get_utf8_char() const {
    return g_utf8_get_char(str_ptr->u8str.c_str());
  }

  [[nodiscard]] bool utf8_validate() const {
    return g_utf8_validate(str_ptr->u8str.c_str(), -1, nullptr);
  }

private:
  std::shared_ptr<str_utf8_t> str_ptr;
};
