#pragma once

#include "RemoteType.h"
#include "pyextpublic.h"

namespace PyExt::Remote {

	class PYEXT_PUBLIC PyMemberDef : private RemoteType
	{

	public: // Constants.
		static const int T_SHORT          =  0;
		static const int T_INT            =  1;
		static const int T_LONG           =  2;
		static const int T_FLOAT          =  3;
		static const int T_DOUBLE         =  4;
		static const int T_STRING         =  5;
		static const int T_OBJECT         =  6;
		static const int T_CHAR           =  7;
		static const int T_BYTE           =  8;
		static const int T_UBYTE          =  9;
		static const int T_USHORT         = 10;
		static const int T_UINT           = 11;
		static const int T_ULONG          = 12;
		static const int T_STRING_INPLACE = 13;
		static const int T_BOOL           = 14;
		static const int T_OBJECT_EX      = 16;
		static const int T_LONGLONG       = 17;
		static const int T_ULONGLONG      = 18;
		static const int T_PYSSIZET       = 19;
		static const int T_NONE           = 20;

	public: // Construction/Destruction.
		explicit PyMemberDef(Offset objectAddress);
		~PyMemberDef();

	public: // Members of the remote type.
		auto name() const -> std::string;
		auto type() const -> int;
		auto offset() const -> SSize;

	};

}
