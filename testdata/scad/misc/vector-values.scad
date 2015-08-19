// Value vector tests.

a1=[0,1,2];
b1=[3,4,5];
c1=a1*b1;
echo(str("Testing vector dot product: ",c1));

d1=[1,0];
echo(str("  Bounds check: ",a1*d1));

m2=[[0,1],[1,0],[2,3]];
v2=[2,3];
p2=m2*v2;
echo(str("Testing matrix * vector: ",p2));

d2=[0,0,1];
echo(str("  Bounds check: ",m2*d2));

m3=[[1,-1],[1,0],[2,3]];
v3=[1,2,3];
p3=v3*m3;
echo(str("Testing vector * matrix: ",p3));

echo(str("  Bounds check: ",m3*v3));

ma4=[ [1,0],[0,1] ];
mb4=[ [1,0],[0,1] ];
echo(str("Testing id matrix * id matrix: ",ma4*mb4));

ma5=[ [1, 0, 1]
     ,[0, 1,-1] ];
mb5=[ [1,0]
     ,[0,1]
     ,[1,1] ];
echo(str("Testing asymmetric matrix * matrix: ",ma5*mb5));
echo(str("Testing alternate asymmetric matrix * matrix: ",mb5*ma5));

echo(str("  Bounds check: ",ma5*ma4));

ma6=[ [ 1, 2 ], undef ];
mb6=[ [ 4 ], [ 5 ] ];
echo(str("Testing matrix * matrix with undef elements: ",ma6*mb6));


cube(1.0);
