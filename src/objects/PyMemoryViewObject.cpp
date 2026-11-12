#include "PyMemoryViewObject.h"

#include "../fieldAsPyObject.h"
#include "../ExtHelpers.h"
#include "../ReprBuilder.h"

#include <engextcpp.hpp>

#include <sstream>
#include <string>
#include <vector>
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
		return linkToSelf(oss.str(), pretty);
	}


	auto PyMemoryViewObject::details() const -> string
	{
		auto joined = [](const vector<SSize>& values) {
			string result;
			for (size_t i = 0; i < values.size(); ++i) {
				result += to_string(values[i]);
				if (i + 1 < values.size())
					result += ", ";
			}
			return result;
		};

		utils::ReprBuilder out(true); //< Details are always printed as DML.
		out.text("exporter: ");
		if (auto exp = exporter())
			out.markup(exp->repr(true));
		else
			out.text("<none>");
		out.text("\n");

		out.text("format: '" + format() + "'\n");
		out.text("itemsize: " + to_string(itemsize()) + "\n");
		out.text("len: " + to_string(length()) + "\n");
		out.text("readonly: "s + (readonly() ? "True" : "False") + "\n");
		out.text("ndim: " + to_string(ndim()) + "\n");
		out.text("shape: (" + joined(shape()) + ")\n");
		out.text("strides: (" + joined(strides()) + ")");
		return out.str();
	}

}
