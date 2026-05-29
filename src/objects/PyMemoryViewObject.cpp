#include "PyMemoryViewObject.h"

#include "../fieldAsPyObject.h"
#include "../ExtHelpers.h"

#include <engextcpp.hpp>

#include <sstream>
#include <string>
using namespace std;

namespace PyExt::Remote {

	PyMemoryViewObject::PyMemoryViewObject(Offset objectAddress)
		: PyVarObject(objectAddress, "PyMemoryViewObject")
	{
	}


	auto PyMemoryViewObject::exporter() const -> unique_ptr<PyObject>
	{
		auto objPtr = remoteType().Field("view").Field("obj").GetPtr();
		return objPtr ? make(objPtr) : nullptr;
	}


	auto PyMemoryViewObject::length() const -> SSize
	{
		auto len = remoteType().Field("view").Field("len");
		return utils::readIntegral<SSize>(len);
	}


	auto PyMemoryViewObject::itemsize() const -> SSize
	{
		auto sz = remoteType().Field("view").Field("itemsize");
		return utils::readIntegral<SSize>(sz);
	}


	auto PyMemoryViewObject::readonly() const -> bool
	{
		auto ro = remoteType().Field("view").Field("readonly");
		return utils::readIntegral<int>(ro) != 0;
	}


	auto PyMemoryViewObject::ndim() const -> int
	{
		auto n = remoteType().Field("view").Field("ndim");
		return utils::readIntegral<int>(n);
	}


	auto PyMemoryViewObject::format() const -> string
	{
		auto fmtField = remoteType().Field("view").Field("format");
		if (fmtField.GetPtr() == 0)
			return {};
		ExtBuffer<char> buff;
		fmtField.Dereference().GetString(&buff);
		return buff.GetBuffer();
	}


	auto PyMemoryViewObject::shape() const -> vector<SSize>
	{
		vector<SSize> dims;
		auto n = ndim();
		if (n <= 0)
			return dims;
		auto shapeField = remoteType().Field("view").Field("shape");
		if (shapeField.GetPtr() == 0)
			return dims;
		for (int i = 0; i < n; ++i) {
			auto elem = shapeField.ArrayElement(i);
			dims.push_back(utils::readIntegral<SSize>(elem));
		}
		return dims;
	}


	auto PyMemoryViewObject::strides() const -> vector<SSize>
	{
		vector<SSize> result;
		auto n = ndim();
		if (n <= 0)
			return result;
		auto stridesField = remoteType().Field("view").Field("strides");
		if (stridesField.GetPtr() == 0)
			return result;
		for (int i = 0; i < n; ++i) {
			auto elem = stridesField.ArrayElement(i);
			result.push_back(utils::readIntegral<SSize>(elem));
		}
		return result;
	}


	auto PyMemoryViewObject::repr(bool pretty) const -> string
	{
		ostringstream oss;
		oss << "<memory at 0x" << hex << offset() << ">";
		auto repr = oss.str();
		if (pretty)
			return utils::link(repr, "!pyobj 0n"s + to_string(offset()));
		return repr;
	}


	auto PyMemoryViewObject::details() const -> string
	{
		ostringstream oss;
		auto exp = exporter();
		oss << "exporter: " << (exp ? exp->repr(true) : "<none>") << "\n";
		oss << "format: '" << format() << "'\n";
		oss << "itemsize: " << itemsize() << "\n";
		oss << "len: " << length() << "\n";
		oss << "readonly: " << (readonly() ? "True" : "False") << "\n";
		oss << "ndim: " << ndim() << "\n";

		auto dims = shape();
		oss << "shape: (";
		for (size_t i = 0; i < dims.size(); ++i) {
			oss << dims[i];
			if (i + 1 < dims.size()) oss << ", ";
		}
		oss << ")\n";

		auto str = strides();
		oss << "strides: (";
		for (size_t i = 0; i < str.size(); ++i) {
			oss << str[i];
			if (i + 1 < str.size()) oss << ", ";
		}
		oss << ")";
		return oss.str();
	}

}
