import win32debug, sys, os

string_obj = "TestString123"
big_string_obj = string_obj * 100
bytes_obj = b"TestBytes123"  # Python 2: str, Python 3: bytes
unicode_obj = u"TestUnicode"  # Python 2: unicode, Python 3: str
byte_array_object = bytearray(b'TestBytearray123')
int_obj = int(1)
long_obj = 123456789012345678901234567890123456789012345678901234567890
all_long_obj = (0, -123456, 123456, 987654321987654321987654321, -987654321987654321987654321)
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

ellipsis_obj = Ellipsis # Need to use the type because Python 2 doesn't support the `...` literal.

slice_obj = slice(1, 10, 2)

range_obj = range(1, 10, 2) # Ranges in Python 2 are just list's but that's Ok for our purposes.

memoryview_obj = memoryview(bytes_obj)

import weakref
class Referent(object): pass
strong_ref = Referent()
weakref_obj = weakref.ref(strong_ref)

class Derived(object):
    def my_super(self): return super(Derived, self) # Use the verbose `super()` to support Python 2
super_obj = Derived().my_super()

# Generators (`yield`) and `next()` work on Py2.6+.
def gen():
	yield 1
	yield 2
generator_obj = gen()
next(generator_obj) # Suspend at the first yield so the iframe member is populated.

# `async def` was added in Python 3.5, so we have to squash any parse errors in older versions.
try:
	exec(
		"async def coro():\n"
		"    return 42\n"
		"coroutine_obj = coro()\n"
	)
except SyntaxError:
	pass

# `async def`+`yield` was added in Python 3.6, so we have to squash any parse errors in older versions.
try:
	exec(
		"async def _agen():\n"
		"    yield 1\n"
		"async_generator_obj = _agen()\n"
	)
except SyntaxError:
	pass

# Generate the dump with all of the objects above in the global scope for the test harness to inspect.
win32debug.dump_process("object_types.dmp")
