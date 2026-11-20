// from https://github.com/openscad/openscad/issues/7013

SmallScalar    = 0;           // [5:10]
SmallVectorFwd = [5,4,3,2];   // [5:10]
SmallVectorRev = [2,3,4,5];   // [5:10]

LargeScalar    = 6;           // [0:5]
LargeVectorFwd = [5,6,7,8];   // [0:5]
LargeVectorRev = [8,7,6,5];   // [0:5]

MixedVectorFwd = [0,1,2,3];   // [1:2]
MixedVectorRev = [3,2,1,0];   // [1:2]

GoodScalar = 3;         // [0:5]
GoodVector = [0,1,2,3]; // [0:5]

FloatingPoint = [0.2, 0.19, 0.111119, 0.1111119]; // [0.5:1]