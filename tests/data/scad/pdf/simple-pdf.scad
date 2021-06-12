n = 20;

p = [
    for (i = [0 : n - 1])
        let(a = 360 / n * i)
            (50 * (i%2) + 30) * [ -sin(a), cos(a) ]
    ];

translate([100, 150]) difference() { polygon(p); circle(20); }
translate([1, 1]) text("Hello World!", 26, font = "Liberation Sans:style=Regular");
