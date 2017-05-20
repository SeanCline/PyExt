#include "RemotePyObject.h"

#include <memory>

// Creates a polymorphic representation of a PyObject in the debuggee's address space.
auto makeRemotePyObject(RemotePyObject::Offset remoteAddress) -> std::unique_ptr<RemotePyObject>;
