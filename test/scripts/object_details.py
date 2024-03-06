import win32debug, sys, os


class D(object):
    """dict"""
    def __init__(self, d1, d2):
        self.d0 = 0
        self.d1 = d1
        self.d2 = d2
        del self.d0  # d0 is still in the object's dict but has no value

    def f(self):
        self.d_uninitialized = 1


class S(object):
    """slots"""
    __slots__ = 'slot1', 'slot2', 'slot_uninitialized'

    def __init__(self, s1, s2):
        self.slot1 = s1
        self.slot2 = s2


class DsubD(D):
    """dict, parent dict"""
    def __init__(self, d1, d2, d3):
        D.__init__(self, d1, d2)
        self.d3 = d3


class SsubS(S):
    """slots, parent slots"""
    __slots__ = 'slot3'

    def __init__(self, s1, s2, s3):
        S.__init__(self, s1, s2)
        self.slot3 = s3


class DsubS(S):
    """dict, parent slots"""
    def __init__(self, s1, s2, d3):
        S.__init__(self, s1, s2)
        self.d3 = d3


class SsubD(D):
    """slots, parent dict"""
    __slots__ = 'slot3'

    def __init__(self, d1, d2, s3):
        D.__init__(self, d1, d2)
        self.slot3 = s3


class SsubDS(D, S):
    """slots, parents dict and slots"""
    __slots__ = 'slot3'

    def __init__(self, d1, d2, s1, s2, s3):
        D.__init__(self, d1, d2)
        S.__init__(self, s1, s2)
        self.slot3 = s3


class NegDictOffset(tuple):
    """inheriting from tuple leads to a negative tp_dictoffset"""

    def __init__(self, tupleValue):
        self.attr = 'test'


d = D(1, 2)
s = S(1, 2)
dsubd = DsubD(1, 2, 3)
ssubs = SsubS(1, 2, 3)
dsubs = DsubS(1, 2, 3)
ssubd = SsubD(1, 2, 3)
ssubds = SsubDS(1, 2, 3, 4, 5)
negDictOffset = NegDictOffset((1, 2, 3))
win32debug.dump_process("object_details.dmp")
