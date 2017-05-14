import win32debug
import inspect

stringObj = "asd123qwe"
bigStringObj = stringObj * 100
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
frameObj = inspect.currentframe()
codeObj = frameObj.f_code

listObj = [stringObj, intObj, longObj]

tupleObj = (stringObj, intObj, longObj)

dictObj = {
	"stringObj": stringObj,
	"intObj": intObj,
	"longObj": longObj,
}

win32debug.dump_process(__file__ + ".dmp")