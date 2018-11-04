echo([each for(i=1) i]);
echo([each for(i=[1,2]) i]);
echo([each for(i=[1,2,[3,4]]) i]);
echo([each for(i=[1,2,[3,4,[5,6]]]) i]);
echo([each each for(i=[1,2,[3,4,[5,6]]]) i]);
echo([each each for(i=[1,2,[3,4,[5,6,[7,8]]]]) i]);
echo([each each each for(i=[1,2,[3,4,[5,6,[7,8]]]]) i]);
