import win32debug, sys, os


class A(object):
    def __init__(self, a):
        self.a = a

    def f_cellvar(self, param1, param2):
        local1 = 5
        # c is cell variable because it is used in f_cellfreevar
        cell1 = {1: 2, 3: 4}

        def f_cellfreevar(param):
            # cell1 is free variable because it is defined outside f_cellfreevar
            # cell2 is cell variable because it is used in f_freevar
            cell2 = param + cell1[1]
            local2 = 6

            def f_freevar():
                x = cell2
                win32debug.dump_process("localsplus_test.dmp")

            try:
                f_freevar()
            except Exception as e:
                # e is not initialized (null in localsplus) when creating the dump!
                pass

        assert 'cell2' in f_cellfreevar.__code__.co_cellvars
        assert 'cell1' in f_cellfreevar.__code__.co_freevars
        assert f_cellfreevar.__code__.co_nlocals == 4  # param, local2, f_freevar, e
        f_cellfreevar(7)


def main(a):
    a.f_cellvar("test", [1,2,3])


main(A(1))
