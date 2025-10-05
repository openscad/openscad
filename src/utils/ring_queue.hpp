#include <cassert>
#include <vector>
#include <optional>

template<class T>
class ring {
  std::vector<std::optional<T>> _slots;
  std::size_t head = 0, tail = 0;

public:
  ring(size_t size) : _slots(size) {}
  
  template<class... Args>
  T& emplace_back(Args&&... args) {
    auto& s = _slots[tail];
    s.emplace(std::forward<Args>(args)...);  // construct in-place
    tail = (tail + 1) % _slots.size();
    if (tail == head) {
      head = (head + 1) % _slots.size();
    }
    return *s;
  }

  void pop_front() {
    _slots[head].reset();        // destroy just this element
    head = (head + 1) % _slots.size();
  }

  T& front() { assert(!empty()); return *_slots[head]; }
  bool empty() const { return head == tail; }
  std::size_t size() const { return (tail + _slots.size() - head) % _slots.size(); }
};
