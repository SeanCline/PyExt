import win32debug


stringObj = "asd123qwe"
intObj = int(1)
longObj = long(123456789012345678901234567890)
floatObj = 3.1415
complexObj = complex(1, -1)
boolTrueObj = True
boolFalseObj = False
noneObj = None
typeObj = dict
notImplementedObj = NotImplemented

listObj = [stringObj, intObj, longObj]

tupleObj = (stringObj, intObj, longObj)

dictObj = {
	"stringObj": stringObj,
	"intObj": intObj,
	"longObj": longObj,
}

win32debug.dump_process(__file__ + ".dmp")