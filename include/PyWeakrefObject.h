#pragma once

#include "PyObject.h"
#include <memory>
#include <string>

namespace PyExt::Remote {

	/// Represents a PyWeakReference (and weakref.proxy / weakref.callable-proxy) in the debuggee's address space.
	class PYEXT_PUBLIC PyWeakrefObject : public PyObject
	{

	public: // Construction/Destruction.
		explicit PyWeakrefObject(Offset objectAddress);

	public: // Members.
		auto referent() const -> std::unique_ptr<PyObject>;
		auto callback() const -> std::unique_ptr<PyObject>;
		auto repr(bool pretty = true) const -> std::string override;

	};

}
