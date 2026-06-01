// Raw-DbgEng typed remote value — replacement for ExtRemoteTyped
// (engextcpp-replacement.md §2/§4). No precompiled header, no engextcpp.
//
// Foundation increment: the scalar/field/navigation core needed to prove parity
// against ExtRemoteTyped. deref()/element()/str()/bytes() are stubbed to throw
// until Phase 2 step 4 (the cast-expression-site reimplementation) needs them.

#include "RemoteValue.h"
#include "DbgEngContext.h"

#include <windows.h>
#include <DbgEng.h>

#include <string>

namespace PyExt::Dbg {

	namespace {
		void check(HRESULT hr, const char* what)
		{
			if (FAILED(hr))
				throw RemoteReadError(std::string(what) + " failed (hr=0x"
					+ [hr] { char b[16]; sprintf_s(b, "%08lx", static_cast<unsigned long>(hr)); return std::string(b); }() + ")");
		}
	}


	RemoteValue::RemoteValue(const DbgEngContext& ctx, std::string_view typeName, Offset address)
		: ctx_(&ctx), address_(address), moduleBase_(0), typeId_(0)
	{
		// Resolve "mod!type" or a bare type name to (module, typeId). Try the name
		// as given first (matches unique cross-module symbols like "_object"); if
		// that fails, qualify with the loaded python module.
		std::string name(typeName);
		ULONG typeId = 0;
		ULONG64 module = 0;
		HRESULT hr = ctx_->symbols()->GetSymbolTypeId(name.c_str(), &typeId, &module);
		if (FAILED(hr)) {
			if (auto qualified = ctx_->qualifyPythonSymbol(typeName)) {
				hr = ctx_->symbols()->GetSymbolTypeId(qualified->c_str(), &typeId, &module);
			}
		}
		check(hr, ("GetSymbolTypeId(" + name + ")").c_str());
		moduleBase_ = module;
		typeId_ = typeId;
	}


	RemoteValue::RemoteValue(const DbgEngContext& ctx, Offset address, std::uint64_t moduleBase, std::uint32_t typeId)
		: ctx_(&ctx), address_(address), moduleBase_(moduleBase), typeId_(typeId)
	{
	}


	auto RemoteValue::hasField(std::string_view name) const -> bool
	{
		std::string field(name);
		ULONG fieldTypeId = 0, offset = 0;
		return SUCCEEDED(ctx_->symbols()->GetFieldTypeAndOffset(
			moduleBase_, typeId_, field.c_str(), &fieldTypeId, &offset));
	}


	auto RemoteValue::field(std::string_view name) const -> std::optional<RemoteValue>
	{
		std::string field(name);
		ULONG fieldTypeId = 0, offset = 0;
		HRESULT hr = ctx_->symbols()->GetFieldTypeAndOffset(
			moduleBase_, typeId_, field.c_str(), &fieldTypeId, &offset);
		if (FAILED(hr))
			return std::nullopt; // expected-absent -> nullopt
		return RemoteValue(*ctx_, address_ + offset, moduleBase_, fieldTypeId);
	}


	auto RemoteValue::tryField(std::initializer_list<std::string_view> names) const -> std::optional<RemoteValue>
	{
		for (auto name : names) {
			if (auto f = field(name))
				return f;
		}
		return std::nullopt;
	}


	auto RemoteValue::baseField(std::string_view name) const -> std::optional<RemoteValue>
	{
		// Python3 tucks base members into a nested `ob_base` struct. Drill down
		// until `name` resolves. Mirrors PyObject::baseField.
		RemoteValue obj = *this;
		while (!obj.hasField(name) && obj.hasField("ob_base")) {
			auto next = obj.field("ob_base");
			if (!next)
				break;
			obj = *next;
		}
		return obj.field(name);
	}


	auto RemoteValue::typeSize() const -> std::size_t
	{
		ULONG size = 0;
		check(ctx_->symbols()->GetTypeSize(moduleBase_, typeId_, &size), "GetTypeSize");
		return size;
	}


	auto RemoteValue::typeName() const -> std::string
	{
		char buf[256] = {};
		ULONG nameSize = 0;
		check(ctx_->symbols()->GetTypeName(moduleBase_, typeId_, buf, sizeof(buf), &nameSize), "GetTypeName");
		return std::string(buf);
	}


	auto RemoteValue::address() const -> Offset { return address_; }


	auto RemoteValue::readInto(void* dst, std::size_t count) const -> void
	{
		ULONG bytesRead = 0;
		HRESULT hr = ctx_->data()->ReadVirtual(address_, dst, static_cast<ULONG>(count), &bytesRead);
		if (FAILED(hr) || bytesRead != count)
			throw RemoteReadError("ReadVirtual short read at 0x"
				+ [a = address_] { char b[24]; sprintf_s(b, "%llx", static_cast<unsigned long long>(a)); return std::string(b); }());
	}


	auto RemoteValue::readScalarBits(std::size_t size) const -> std::uint64_t
	{
		if (size == 0 || size > 8)
			throw RemoteReadError("RemoteValue::readScalarBits: bad size " + std::to_string(size));
		std::uint64_t raw = 0; // zero-extended
		readInto(&raw, size);
		return raw;
	}


	auto RemoteValue::ptr() const -> Offset
	{
		return readScalarBits(static_cast<std::size_t>(ctx_->pointerSize()));
	}


	auto RemoteValue::bytes(std::size_t count) const -> std::vector<std::byte>
	{
		std::vector<std::byte> buf(count);
		if (count != 0)
			readInto(buf.data(), count);
		return buf;
	}


	// --- navigation stubs: filled in Phase 2 step 4 (cast-expression sites) ---

	auto RemoteValue::deref() const -> std::optional<RemoteValue>
	{
		throw RemoteReadError("RemoteValue::deref not yet implemented (Phase 2 step 4)");
	}

	auto RemoteValue::element(std::size_t /*i*/) const -> RemoteValue
	{
		throw RemoteReadError("RemoteValue::element not yet implemented (Phase 2 step 4)");
	}

	auto RemoteValue::str() const -> std::string
	{
		throw RemoteReadError("RemoteValue::str not yet implemented (Phase 2 step 4)");
	}

}
