// See diagrams:
// https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Primitive_Solids#polyhedron

// One quad (front) has two duplicate vertices
polyhedron([
  [  0,  0,  0 ],  //0
  [ 10,  0,  0 ],  //1
  [ 10,  7,  0 ],  //2
  [  0,  7,  0 ],  //3
  [  0,  0,  0 ],  //4
  [ 10,  0,  0 ],  //5
  [ 10,  7,  5 ],  //6
  [  0,  7,  5 ]
], [
  [0,1,2,3],  // bottom
  [4,5,1,0],  // front
  [7,6,5,4],  // top
  [5,6,2,1],  // right
  [6,7,3,2],  // back
  [7,4,0,3]
]);

// Two quads (front, right) have duplicate vertices
translate([15, 0, 0])
  polyhedron([
    [  0,  0,  0 ],  //0
    [ 10,  0,  0 ],  //1
    [ 10,  7,  0 ],  //2
    [  0,  7,  0 ],  //3
    [  0,  0,  5 ],  //4
    [ 10,  0,  0 ],  //5
    [ 10,  7,  5 ],  //6
    [  0,  7,  5 ]
  ], [
    [0,1,2,3],  // bottom
    [4,5,1,0],  // front
    [7,6,5,4],  // top
    [5,6,2,1],  // right
    [6,7,3,2],  // back
    [7,4,0,3]
  ]);