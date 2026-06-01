#pragma once

// The engextcpp-free replacement for ExtRemoteTyped (engextcpp-replacement.md
// §2/§4). A RemoteValue is a typed view over an address in the debuggee, backed
// by IDebugSymbols3 (type/field metadata) + IDebugDataSpaces4 (ReadVirtual).
//
// Error-handling contract (§4) — the whole point of the rewrite:
//   * expected-absent  -> std::optional == nullopt
//       - field name not present on this type      (field/tryField)
//       - pointer/field value is 0 (null)           (via deref returning nullopt)
//   * environment-broken -> throws RemoteReadError
//       - address not mapped in the dump (ReadVirtual fails)
//       - symbol/type cannot be resolved
//       - requested width doesn't match the field size
//
// Rule of thumb: "this Python version doesn't have that field" is data and
// returns nullopt; "the dump is truncated / symbols are missing" is an error
// and throws. This keeps the common version-dispatch path exception-free while
// refusing to silently paper over a broken dump (the trap called out in
// plan.md).

#include "pyextpublic.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace PyExt::Dbg {

	class DbgEngContext;

	class PYEXT_PUBLIC RemoteReadError : public std::runtime_error {
	public:
		using std::runtime_error::runtime_error;
	};

	class PYEXT_PUBLIC RemoteValue {
	public:
		using Offset = std::uint64_t;

		// Resolve `typeName` (optionally module-qualified) and view it at
		// `address`. Throws RemoteReadError if the type can't be resolved.
		RemoteValue(const DbgEngContext& ctx, std::string_view typeName, Offset address);

		// --- structure navigation (expected-absent -> nullopt) ---

		auto hasField(std::string_view name) const -> bool;

		// nullopt if the field isn't present on this type.
		auto field(std::string_view name) const -> std::optional<RemoteValue>;

		// First present name from the list — concentrates version renames in one
		// place (subsumes plan.md item 3's tryField). nullopt if none present.
		auto tryField(std::initializer_list<std::string_view> names) const -> std::optional<RemoteValue>;

		// Walks ob_base until `name` resolves (replaces PyObject::baseField).
		auto baseField(std::string_view name) const -> std::optional<RemoteValue>;

		// Follow a pointer-typed value. nullopt if the pointer is null;
		// throws if the pointee can't be read.
		auto deref() const -> std::optional<RemoteValue>;

		// element i of an array-typed value.
		auto element(std::size_t i) const -> RemoteValue;

		// --- scalar reads (environment-broken -> throw) ---

		// Width-correct integral read (replaces utils::readIntegral): reads the
		// field's actual size in bytes and sign-/zero-extends per `T`.
		template <std::integral T>
		auto as() const -> T
		{
			const auto sz = typeSize();
			std::uint64_t raw = readScalarBits(sz); // zero-extended, sz in [1,8]
			if constexpr (std::is_signed_v<T>) {
				if (sz < 8) {
					const std::uint64_t signbit = std::uint64_t{1} << (sz * 8 - 1);
					if (raw & signbit)
						raw |= ~((std::uint64_t{1} << (sz * 8)) - 1); // sign-extend
				}
				return static_cast<T>(static_cast<std::int64_t>(raw));
			}
			return static_cast<T>(raw);
		}

		// The pointer value held here (replaces GetPtr()). Throws if this isn't
		// a pointer-sized readable location.
		auto ptr() const -> Offset;

		// NUL-terminated string at this location (replaces GetString).
		auto str() const -> std::string;

		// --- raw access (the "raw byte access if necessary" requirement) ---

		auto bytes(std::size_t count) const -> std::vector<std::byte>;

		// Reads numElements*sizeof(Elem) bytes starting at this location.
		// Replaces utils::readArray. Throws on short read.
		template <typename Elem>
		auto array(std::size_t numElements) const -> std::vector<Elem>
		{
			std::vector<Elem> buf(numElements);
			if (numElements != 0)
				readInto(buf.data(), numElements * sizeof(Elem));
			return buf;
		}

		// --- metadata ---

		auto address() const -> Offset;
		auto typeName() const -> std::string;
		auto typeSize() const -> std::size_t;

	private:
		// Internal ctor used by field()/element()/deref() once the module+typeId
		// are already known (no symbol lookup).
		RemoteValue(const DbgEngContext& ctx, Offset address, std::uint64_t moduleBase, std::uint32_t typeId);

		// Reads `size` bytes (1..8) at address_ into a zero-extended u64. Throws
		// RemoteReadError on short read. Used by as<T>()/ptr().
		auto readScalarBits(std::size_t size) const -> std::uint64_t;

		// Raw ReadVirtual of `count` bytes at address_ into dst. Throws on short read.
		auto readInto(void* dst, std::size_t count) const -> void;

		const DbgEngContext* ctx_;
		Offset address_;
		std::uint64_t moduleBase_;
		std::uint32_t typeId_;
	};

}
