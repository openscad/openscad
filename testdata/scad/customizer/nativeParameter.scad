@Parameter(12)
x1 = 1;
@Description("normal description")
@Parameter(12)
x2 = 1;
@Description("normal1 description")
@Description("normal2 description")
@Parameter(12)
x3 = 1;
@Parameter("ADAD")
@Parameter([1 : 12])
@Parameter(12)
x4 = 1;
@Group("Global")
@Description("normal description")
@Parameter([1 : 12])
x5 = 1;
@Description("normal description")
@Group("Global")
@Parameter([1 : 2 : 12])
x6 = 1;
@Description("normal description")
x7 = 1;
@Group("Global")
x8 = 1;
x9 = 1;
x10 = 1;
x11 = 1;
@Group("Global")
@Parameter([12])
x12 = 1;
@Parameter([[10, "Small"], [20, "Medium"], [30, "Large"]])
x13 = 10;
@Parameter([[10, 100], [20, 101], [30, 102]])
x14 = 10;
@Parameter("parameter")
x15 = 10;
@Parameter([0, 1, 2, 3])
x16 = 10;
@Parameter("parameter")
x17 = "text";
@Parameter(["foo", "bar", "baz"])
x18 = "text";
@Parameter([[0, "text"], [1, "foo"], [2, "bar"], [3, "hello"]])
x19 = "text";
@Parameter([["foo", 10], ["bar", 10], ["baz", 30]])
x20 = "text";
@Parameter([["foo", "yes"], ["bar", "no"], ["baz", "mgiht"]])
x21 = "text";
@Parameter([23, 4])
x22 = [12, 34];
@Parameter([23, 4, 23, 4, 45])
x23 = [12, 34];
@Parameter([23, 4, 2, 3, 4, 6])
x24 = [12, 34, 2, 3, 41, 23];
@Parameter("end parameter")
x27 = 12;

