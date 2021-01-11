import win32debug, sys, os

string_obj = "TestString123"
big_string_obj = string_obj * 100
bytes_obj = b"TestBytes123"  # Python 2: str, Python 3: bytes
unicode_obj = u"TestUnicode"  # Python 2: unicode, Python 3: str
byte_array_object = bytearray(b'TestBytearray123')
int_obj = int(1)
long_obj = 123456789012345678901234567890123456789012345678901234567890
float_obj = 3.1415
complex_obj = complex(1.5, -2.25)
bool_true_obj = True
bool_false_obj = False
none_obj = None
type_obj = dict
not_implemented_obj = NotImplemented

def test_function(x):
    """Some DocString"""
    return x*x

func_obj = test_function

list_obj = [string_obj, int_obj, long_obj]

tuple_obj = (string_obj, int_obj, long_obj)

set_obj = { string_obj, int_obj, long_obj }

dict_obj = {
	"string_obj": string_obj,
	"int_obj": int_obj,
	"long_obj": long_obj,
}

win32debug.dump_process("object_types.dmp")
