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

v = timer_new("auto", start=true);
va = timer_stop(v, delete=true);

w = timer_new(start=true);
wa = timer_stop(w, undef, delete=true);
zx = timer_new("named-id-tail", start=true);
zy = timer_stop(fmt_str="done", output=true, timer_id=zx, delete=true);

z = timer_new("mixed", start=true, "CPU");
