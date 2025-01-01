#pragma once


// FIXME: Default constructor Response()
enum class Response {ContinueTraversal, AbortTraversal, PruneTraversal};

class State;

class BaseVisitor
{
public:
  virtual ~BaseVisitor() = default;
};

template <class T>
class Visitor
{
public:
  virtual Response visit(State& state, const T&) = 0;
};

class BaseVisitable
{
public:
  virtual ~BaseVisitable() = default;
  virtual Response accept(State&, BaseVisitor&) const = 0;
protected:
  template <class T>
  static Response acceptImpl(State& state, const T& node, BaseVisitor& visitor) {
    if (auto *p = dynamic_cast<Visitor<T> *>(&visitor)) {
      return p->visit(state, node);
    }
    // FIXME: If we want to allow for missing nodes in visitors, we need
    // to handle it here, e.g. by calling some handler.
    // See e.g. page 225 of Alexandrescu's "Modern C++ Design"
    return Response::AbortTraversal;
  }
};

#define VISITABLE() \
        Response accept(State &state, BaseVisitor &visitor) const override { \
          return acceptImpl(state, *this, visitor); \
        }
