https://gist.github.com/ochafik/8aa845c8b9ee3a241e771dda99a2d5ef

# [fast-union] Heuristic to fast-union large locally-overlapping unions


Notice the N=3 vs. N=5 below. All test on a MacBook Pro w/ 2.6GHz Core i7.

## Baseline
time openscad spheres_single_for_loop.scad  -o out.stl -DN=3 -Doverlap=true # 52s
time openscad spheres_nested_for_loops.scad -o out.stl -DN=3 -Doverlap=true # 45s
time openscad spheres_single_for_loop.scad  -o out.stl -DN=5 -Doverlap=true # 3min32
time openscad spheres_nested_for_loops.scad -o out.stl -DN=5 -Doverlap=true # 2min41

## fast-union before this commit
time openscad spheres_single_for_loop.scad  -o out.stl --enable=fast-union -DN=3 -Doverlap=true # 48s
time openscad spheres_nested_for_loops.scad -o out.stl --enable=fast-union -DN=3 -Doverlap=true # 47s
time openscad spheres_single_for_loop.scad  -o out.stl --enable=fast-union -DN=5 -Doverlap=true # 2m38
> time openscad spheres_nested_for_loops.scad -o out.stl --enable=fast-union -DN=5 -Doverlap=true # 2m13

## fast-union after this commit
time openscad spheres_single_for_loop.scad  -o out.stl --enable=fast-union -DN=3 -Doverlap=true # 35s
time openscad spheres_nested_for_loops.scad -o out.stl --enable=fast-union -DN=3 -Doverlap=true # 36s
time openscad spheres_single_for_loop.scad  -o out.stl --enable=fast-union -DN=5 -Doverlap=true # 1m42s
time openscad spheres_nested_for_loops.scad -o out.stl --enable=fast-union -DN=5 -Doverlap=true # 1m46

### spheres_single_for_loop.scad

  N = 3;
  overlap = false;

  union() {
    for (i=[0:N-1], j=[0:N-1])
      translate([i,j,0])
        sphere(d=(overlap ? 1.1 : 0.9), $fn=50);
  }

## spheres_nested_for_loops.scad

  N = 3;
  overlap = false;

  union() {
    for (i=[0:N-1]) translate([i,0,0])
    for (j=[0:N-1]) translate([0,j,0])
      sphere(d=(overlap ? 1.1 : 0.9), $fn=50);
  }



// # Naive LazyGeometry::reduceFastUnions heuristic makes it worse for now (needs more work):
// time openscad --enable=fast-union chainmail.scad -o chainmail_fast_heuristic.stl
//     891 sec




