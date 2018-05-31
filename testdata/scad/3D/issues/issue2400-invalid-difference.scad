// The result should be empty, but was incorrectly returning the d=2 cylinder as the final result.
render() difference() {
        rotate(0)
            cylinder(d = 0, h = 1);

        cylinder(d = 2, h = 10, center = true);
    }
