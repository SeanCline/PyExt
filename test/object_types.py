import win32debug, sys, os

stringObj = "TestString123"
bigStringObj = stringObj * 100
bytesObj = b"TestBytes123"
byteArrayObject = bytearray(b'TestBytearray123')
intObj = int(1)
longObj = 123456789012345678901234567890123456789012345678901234567890
floatObj = 3.1415
complexObj = complex(1.5, -2.25)
boolTrueObj = True
boolFalseObj = False
noneObj = None
typeObj = dict
notImplementedObj = NotImplemented
funcObj = lambda x: x*x

listObj = [stringObj, intObj, longObj]

tupleObj = (stringObj, intObj, longObj)

dictObj = {
	"stringObj": stringObj,
	"intObj": intObj,
	"longObj": longObj,
}

pythonVersionStr = ".".join(str(x) for x in sys.version_info[:3])
fileNameStr = os.path.splitext(os.path.basename(__file__))[0]
bittessStr = 'x64' if sys.maxsize > 2**32 else 'x86'
win32debug.dump_process('.'.join([fileNameStr, pythonVersionStr, bittessStr, "dmp"]))