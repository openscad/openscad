t = timer_new();
ta = timer_start(t);
tb = timer_elapsed(t, undef);
tc = timer_stop(t);
td = timer_elapsed(t, output=true);
te = timer_clear(t);
tf = timer_delete(t);

u = timer_new("cpu", "CPU");
ua = timer_start(u);
ub = timer_stop(u, "timer {n} {f} μs");
uc = timer_delete(u);
