"""
Regression test for issue #601: asyncio segfault in embedded Python.

Before the fix, running any asyncio coroutine inside PythonSCAD's embedded
interpreter would SIGSEGV on the first `await`, because pyfunctions.cc
defined an unconditional polyfill for `PyDict_SetDefaultRef` that never
populated *result. The dynamic linker resolved _asyncio.so's reference to
that broken polyfill (RTLD prefers the main executable), so enter_task()
ended up doing Py_DECREF on uninitialized stack memory.

This test runs a tiny asyncio coroutine and prints a deterministic message.
It SIGSEGVs on the unfixed binary and prints "asyncio_ok" on the fixed one.
"""
import asyncio


async def _main():
    await asyncio.sleep(0)
    return "asyncio_ok"


print(asyncio.run(_main()))
