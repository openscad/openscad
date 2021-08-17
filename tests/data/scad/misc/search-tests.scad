// string searches

simpleSearch1=search("a","abcdabcd");
echo(str("Characters in string (\"a\"): ",simpleSearch1));

simpleSearch2=search("adeq","abcdeabcd",0);
echo(str("Characters in string (\"adeq\"): ",simpleSearch2));

sTable1=[ ["a",1],["b",2],["c",3],["d",4],["a",5],["b",6],["c",7],["d",8],["e",9] ];
s1= search("abe",sTable1);
echo(str("Default string search (\"abe\"): ",s1));

sTable2=[ ["a",1],["b",2],["c",3],["d",4],["a",5],["b",6],["c",7],["d",8],["e",9],["a",10],["a",11] ];
s2= search("abe",sTable2,0);
echo(str("Return all matches for string search (\"abe\"): ",s2));

sTable3=[ ["a",1],["b",2],["c",3],["d",4],["a",5],["b",6],["c",7],["d",8],["e",9],["a",10],["a",11] ];
s3= search("abe",sTable3,2);
echo(str("Return up to 2 matches for string search (\"abe\"): ",s3));

sTable4=[ [1,"a",[20]],[2,"b",21],[3,"c",22],[4,"d",23],[5,"a",24],[6,"b",25],[7,"c",26],[8,"d",27],[9,"e",28],[10,"a",29],[11,"a",30] ];
s4= search("aebe",sTable4,2,1);
echo(str("Return up to 2 matches for string search; alternate columns (\"aebe\"): ",s4));

// s5= search("abe",sTable4,2,1,3); // bounds checking needs fixing.
// echo(str("Return up to 2 matches for string search; alternate columns: ",s4));

// Value searches
vTable1=[1,2,3];
v1 = search(3, vTable1);
echo(str("Default value search (3): ", v1));
vTable1=[1,2,3];
v2 = search(4, vTable1);
echo(str("Value not found (4): ", v2));
vTable2=[[0,1],[1,2],[2,3]];
v3 = search([[1,2]], vTable2);
echo(str("Value vector ([1,2]): ", v3));

// number searches
nTable1=[ [1,"a"],[3,"b"],[2,"c"],[4,"d"],[1,"a"],[7,"b"],[2,"c"],[8,"d"],[9,"e"],[10,"a"],[1,"a"] ];
n1 = search(7,nTable1);
echo(str("Default number search (7): ",n1));
n2 = search(1,nTable1,0);
echo(str("Return all matches for number search (1): ",n2));
n3 = search(1,nTable1,2);
echo(str("Return up to 2 matches for number search (1): ",n3));

// list searches
lTable1=[ [1,"a"],[3,"b"],[2,"c"],[4,"d"],[1,"a"],[7,"b"],[2,"c"],[8,"d"],[9,"e"],[10,"a"],[1,"a"] ];
lSearch1=[1,3,1000];
l1=search(lSearch1,lTable1);
echo(str("Default list number search (",lSearch1,"): ",l1));

lTable2=[ ["cat",1],["b",2],["c",3],["dog",4],["a",5],["b",6],["c",7],["d",8],["e",9],["apple",10],["a",11] ];
lSearch2=["b","zzz","a","c","apple","dog"];
l2=search(lSearch2,lTable2);
echo(str("Default list string search (",lSearch2,"): ",l2));

lTable3=[ ["cat",1],["b",2],["c",3],[4,"dog"],["a",5],["b",6],["c",7],["d",8],["e",9],["apple",10],["a",11] ];
lSearch3=["b",4,"zzz","c","apple",500,"a",""];
l3=search(lSearch3,lTable3);
echo(str("Default list mixed search (",lSearch3,"): ",l3));

l4=search(lSearch3,lTable3,0);
echo(str("Return all matches for mixed search (",lSearch3,"): ",l4));

lSearch5=[1,"zz","dog",500,11];
l5=search(lSearch5,lTable3,0,1);
echo(str("Return all matches for mixed search; alternate columns (",lSearch5,"): ",l5));

// causing warnings
lTableW1=[ ["a",1],123 ];
echo(search("a", lTableW1, num_returns_per_match=0)); 

lTableW2=[ ["a",1],"string" ];
echo(search("a", lTableW2, num_returns_per_match=0)); 

lTableW3=[ ["b",1] ];
echo(search("a", lTableW3, num_returns_per_match=0)); 

lTableW4=[ ["a",1] ];
echo(search("abcd", lTableW4, num_returns_per_match=0)); 

echo(search("abcd", "xyz", num_returns_per_match=0)); 

lTableW5=[ ["a",1],undef,1/0,-1/0 ];
echo(search("a", lTableW5, num_returns_per_match=0)); 

lTableW6=[ ["a",1],-1/0];
echo(search("a", lTableW6, num_returns_per_match=0)); 

// for completeness
cube(1.0);
