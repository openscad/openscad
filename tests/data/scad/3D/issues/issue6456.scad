// Issue 6456 was that an empty group() was erroneously
// pruned out of some expressions where it was necessary for
// correct semantics.  This led to erroneous cache matches.

// We test here for erroneous matches in known cases.
// Note that we cannor directly test for erroneous
// mismatches, because a cache mismatch simply means that
// we unnecessarily process the subtree again, with the
// same result.

// One might think that the render() calls are unnecessary
// if you test with a render rather than a preview, but
// for not-yet-investigated reasons removing the first
// render() in a set causes the bad cases to succeed even on
// builds where the bug is present.

// Test set 1:  difference
// Setup:  a cube that is the only child of a difference.
translate([0,0,0]) {
    render() {
        difference() {
            cube();
        }
    }
}
// Test 1A:  difference a cube from an empty group.
// The result should be empty, but with 6456 the group()
// is erroneously pruned from the cache key, and as a
// result this subtree matches teh cube above and that
// cached cube is added to the model.
translate([2,0,0]) {
    render() {
        difference() {
            group();
            cube();
        }
    }
}

// Test 1B:  As for test 1A, but with the first child being
// a two-level empty group.  It seems unlikely that a bug
// would erroneously prune a more complex structure while
// leaving a trivial empty group() alone, but it's easy
// to test.
translate([4,0,0]) {
    render() {
        difference() {
            group() group();
            cube();
        }
    }
}

// Test set 2:  intersection
// Setup:  a cube that is the only child of an intersection.
translate([0,2,0]) {
    render() {
        intersection() {
            cube();
        }
    }
}

// Test 2A:  intersect an empty group with a cube.
// This should yield an empty result, but as above with the
// bug in place the group() is pruned out and as a result
// this subtree matches the setup cube above and that cube
// is added to the model.
translate([2,2,0]) {
    render() {
        intersection() {
            group();
            cube();
        }
    }
}

// Test 2B:  As for 2A above, test with a two-level empty
// group().
translate([4,2,0]) {
    render() {
        intersection() {
            group() group();
            cube();
        }
    }
}

// Test 2C:  Reverse the order of the children.
translate([6,2,0]) {
    render() {
        intersection() {
            cube();
            group();
        }
    }
}
