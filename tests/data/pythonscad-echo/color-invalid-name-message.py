"""Regression for invalid color-name exception messages."""
from openscad import cube


def expect_message(label, fn):
    try:
        fn()
    except Exception as e:
        print(f'{label}: {type(e).__name__}: {e}')
    else:
        print(f"{label}: NO EXCEPTION")


c = cube(1)
expect_message("color invalid name", lambda: c.color("notAName"))
expect_message("color invalid nul name", lambda: c.color("Br\0on"))
expect_message("repair invalid name", lambda: c.repair(color="notAName"))
expect_message("repair invalid nul name", lambda: c.repair(color="Br\0on"))
