ra = timer_run("run-a", function() 42, fmt_str="run {n} i={i}", iterations=3);
rb = timer_run("run-b", function(a, b) a + b, args=1, 2, fmt_str="sum {n} i={i}", iterations=2);
rc = timer_run("run-c", function(a, b, c) str(a, b, c), args=1, 2, "x", fmt_str="argc {i}");
