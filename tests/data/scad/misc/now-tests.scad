m0 = now();
m1 = now();
echo("monotonic_non_decreasing", m1 >= m0);

bad_pos = now("CPU");
bad_named = now(type="monotonic");
