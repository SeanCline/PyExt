#include "RemotePyObject.h"

#include <memory>

// Creates a polymorphic representation of a PyObject in the debuggee's address space.
std::unique_ptr<RemotePyObject> makeRemotePyObject(RemotePyObject::Offset remoteAddress);