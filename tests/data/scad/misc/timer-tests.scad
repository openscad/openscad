t = timer_new();
timer_start(t);
timer_elapsed(t);
timer_stop(t);
timer_format(timer_elapsed(t));
timer_clear(t);
timer_delete(t);

u = timer_new("cpu", "CPU");
timer_start(u);
timer_stop(u);
timer_delete(u);
