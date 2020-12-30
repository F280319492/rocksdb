#pragma once


#include <iostream>
#include "rocksdb/status.h"

namespace rocksdb {

class Context
{
public:
  Status f_s;
  Context(const Context& other);
  const Context& operator=(const Context& other);

 protected:
  virtual void finish(Status s) = 0;

 public:
  Context() {}
  virtual ~Context() {}       // we want a virtual destructor!!!
  virtual void complete(Status s) {
    finish(s);
    delete this;
  }
  virtual void complete_without_del(Status s) {
    finish(s);
  }
};
}  // namespace rocksdb
