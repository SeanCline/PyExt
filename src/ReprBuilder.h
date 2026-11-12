#pragma once

#include <string>
#include <string_view>

namespace utils {

	/// Builds the text shown for an object, either as plain text or as DML (Debugger Markup Language).
	///
	/// In DML the characters `<`, `>`, `&`, and `"` are markup, and Python reprs are full of them:
	/// "<function <lambda>>", "<genexpr>", a memoryview format of "<i". Text that isn't escaped is
	/// silently swallowed by the debugger as an unknown tag. This class makes escaping the default,
	/// so that each repr() only has to say which of its pieces are already markup.
	class ReprBuilder
	{

	public:
		/// Produces DML when `dml` is set, and plain text otherwise.
		explicit ReprBuilder(bool dml);

		/// Appends literal text, escaping it when producing DML.
		auto text(std::string_view str) -> ReprBuilder&;

		/// Appends something that is already in this builder's output format,
		/// such as the result of a nested object's repr() called with the same `pretty` flag.
		auto markup(std::string_view str) -> ReprBuilder&;

		/// Appends text that runs a debugger command when clicked. In plain text, only the text is appended.
		auto link(std::string_view text, std::string_view command, std::string_view tooltip = {}) -> ReprBuilder&;

		auto str() const -> std::string;

	private:
		bool dml_;
		std::string out_;

	};

}
