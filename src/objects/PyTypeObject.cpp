#include "PyTypeObject.h"

#include "../fieldAsPyObject.h"
#include "../ExtHelpers.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;

namespace PyExt::Remote {

	PyTypeObject::PyTypeObject(Offset objectAddress)
		: PyVarObject(objectAddress, "PyTypeObject")
	{
	}


	auto PyTypeObject::name() const -> string
	{
		ExtBuffer<char> buff;
		remoteType().Field("tp_name").Dereference().GetString(&buff);
		return buff.GetBuffer();
	}


	auto PyTypeObject::documentation() const -> string
	{
		ExtBuffer<char> buff;
		auto doc = remoteType().Field("ob_type").Field("tp_doc");
		if (doc.GetPtr() == 0)
			return {};

		doc.Dereference().GetString(&buff);
		return buff.GetBuffer();
	}


	auto PyTypeObject::members() const -> vector<unique_ptr<PyMemberDef>>
	{
		vector<unique_ptr<PyMemberDef>> members;
		auto tp_members = remoteType().Field("tp_members");
		if (tp_members.GetPtr() != 0) {
			for (int i = 0; ; ++i) {
				auto elem = tp_members.ArrayElement(i);
				if (elem.Field("name").GetPtr() == 0)
					break;
				members.push_back(make_unique<PyMemberDef>(elem.GetPointerTo().GetPtr()));
			}
		}
		return members;
	}


	auto PyTypeObject::dictOffset() const -> SSize
	{
		auto dictOffset = remoteType().Field("tp_dictoffset");
		return utils::readIntegral<SSize>(dictOffset);
	}


	auto PyTypeObject::mro() const -> unique_ptr<PyTupleObject>
	{
		return utils::fieldAsPyObject<PyTupleObject>(remoteType(), "tp_mro");
	}


	// This isn't really the best place for this method, but it's useful for factories to know
	// which type to construct when a Python3 type differs from a Python 2 type of the same name.
	auto PyTypeObject::isPython2() const -> bool
	{
		return !remoteType().HasField("ob_base");
	}


	auto PyTypeObject::repr(bool pretty) const -> string
	{
		string repr = "<class '" + name() + "'>";
		if (pretty)
			return utils::link(repr, "!pyobj 0n"s + to_string(offset()));
		return repr;
	}

}