/****************************************************************************
**
** OpenSCAD (www.openscad.org)
** Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
**                         Marius Kintel <marius@kintel.net>
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

#include <cstddef>
#include <unordered_map>
#include "utils/printutils.h"

template <class Key, class T>
class Cache
{
  struct Node {
    inline Node() : keyPtr(nullptr), t(nullptr), c(0), p(nullptr), n(nullptr) {
    }
    inline Node(T * data, size_t cost) : keyPtr(nullptr), t(data), c(cost), p(nullptr), n(nullptr) {
    }
    const Key *keyPtr; T *t; size_t c; Node *p, *n;
  };
  using map_type = typename std::unordered_map<Key, Node>;
  using iterator_type = typename map_type::iterator;
  using value_type = typename map_type::value_type;

  std::unordered_map<Key, Node> hash;
  Node *f, *l;
  void *unused{nullptr};
  size_t mx, total{0};

  inline void unlink(Node& n) {
    if (n.p) n.p->n = n.n;
    if (n.n) n.n->p = n.p;
    if (l == &n) l = n.p;
    if (f == &n) f = n.n;
    total -= n.c;
    T *obj = n.t;
    hash.erase(*n.keyPtr);
    delete obj;
  }
  inline T *relink(const Key& key) {
    auto i = hash.find(key);
    if (i == hash.end()) return nullptr;

    Node& n = i->second;
    if (f != &n) {
      if (n.p) n.p->n = n.n;
      if (n.n) n.n->p = n.p;
      if (l == &n) l = n.p;
      n.p = nullptr;
      n.n = f;
      f->p = &n;
      f = &n;
    }
    return n.t;
  }

public:
  inline explicit Cache(size_t maxCost = 100)
    : f(nullptr), l(nullptr), mx(maxCost) { }
  inline ~Cache() { clear(); }

  [[nodiscard]] inline size_t maxCost() const { return mx; }
  void setMaxCost(size_t m) { mx = m; trim(mx); }
  [[nodiscard]] inline size_t totalCost() const { return total; }

  [[nodiscard]] inline size_t size() const { return hash.size(); }
  [[nodiscard]] inline bool empty() const { return hash.empty(); }

  void clear() {
    while (f) {
      delete f->t; f = f->n;
    }
    hash.clear();
    l = nullptr;
    total = 0;
  }

  bool insert(const Key& key, T *object, size_t cost);
  T *object(const Key& key) const { return const_cast<Cache<Key, T> *>(this)->relink(key); }
  inline bool contains(const Key& key) const { return hash.find(key) != hash.end(); }
  T *operator[](const Key& key) const { return object(key); }

  bool remove(const Key& key);
  T *take(const Key& key);

private:
  void trim(size_t m);
};

template <class Key, class T>
inline bool Cache<Key, T>::remove(const Key& key)
{
  auto i = hash.find(key);
  if (i == hash.end()) {
    return false;
  } else {
    unlink(i->second);
    return true;
  }
}

template <class Key, class T>
inline T *Cache<Key, T>::take(const Key& key)
{
  iterator_type i = hash.find(key);
  if (i == hash.end()) return 0;

  Node& n = *i;
  T *t = n.t;
  n.t = 0;
  unlink(n);
  return t;
}

template <class Key, class T>
bool Cache<Key, T>::insert(const Key& akey, T *aobject, size_t acost)
{
  remove(akey);
  if (acost > mx) {
    delete aobject;
    return false;
  }
  trim(mx - acost);
  Node node(aobject, acost);
  hash[akey] = node;
  auto i = hash.find(akey);
  total += acost;
  Node *n = &i->second;
  n->keyPtr = &i->first;
  if (f) f->p = n;
  n->n = f;
  f = n;
  if (!l) l = f;
  return true;
}

template <class Key, class T>
void Cache<Key, T>::trim(size_t m)
{
  Node *n = l;
  while (n && total > m) {
    Node *u = n;
    n = n->p;
#ifdef DEBUG
    LOG("Trimming cache: %1$s (%2$d bytes)", u->keyPtr->substr(0, 40), u->c);
#endif
    unlink(*u);
  }
}
