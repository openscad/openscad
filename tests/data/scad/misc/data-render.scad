data = render()
        cube(10);
echo(data);

data2 = render() {
            translate([10,10,10])
                cube(10);
        };
echo(data2);

data3 = render()
            circle();

data4 = render() {
            linear_extrude(33, true)
                polygon(data3.points, data3.paths);
        };
echo(data4);
