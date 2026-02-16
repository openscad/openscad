t = timer_new("iters", start=true);
a = timer_elapsed(t, "iterations={i}", 5, output=true);
b = timer_elapsed(t, undef, 5);
assert(is_num(b));
c = timer_stop(t, "stop iterations={i}", 7, output=true, delete=true);

d = timer_new(start=true);
e = timer_stop(d, undef, 3, delete=true);
assert(is_num(e));
