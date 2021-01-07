import win32debug, sys, os


class A:
    def __init__(self, a):
        self.a = a

    def f(self, a, b):
        c = {1: 2, 3: 4}
        win32debug.dump_process("localsplus_test.dmp")


class B:
    def __init__(self, b):
        self.b = b


def main(a):
    a.f("test", [1,2,3])


main(A(B(1)))
