// string searches
function search_vector(vec,table,col=0) = [for(i=[0:len(vec)-1]) search(vec[i],table,col)];

simpleSearch1=search("a","abcdabcd");
echo(str("Characters in string (\"a\"): ",simpleSearch1));

simpleSearch2=search("abc","abcdeabcd");
echo(str("Substring in string (\"abc\"): ",simpleSearch2));

sTable1=[ ["a",1],["b",2],["c",3],["d",4],["a",5],["b",6],["c",7],["d",8],["e",9] ];
s1= search("a",sTable1);
echo(str("Default string search table (\"a\"): ",s1));

sTable2=[ ["a",1],["b",2],["c",3],["d",4],["a",5],["b",6],["c",7],["d",8],["e",9],["a",10],["a",11] ];
s2= search("b",sTable2);
echo(str("Default string search table (\"b\"): ",s2));

sTable3=[ ["a",1],["b",2],["c",3],["d",4],["a",5],["b",6],["c",7],["d",8],["e",9],["a",10],["a",11] ];
s3= search("c",sTable3);
echo(str("Default string search table (\"c\"): ",s3));

sTable4=[ [1,"a",[20]],[2,"b",21],[3,"c",22],[4,"d",23],[5,"a",24],[6,"b",25],[7,"c",26],[8,"d",27],[9,"e",28],[10,"a",29],[11,"a",30] ];
s4= search("a",sTable4,1);
echo(str("Return all matches for string search; alternate columns (\"a\"): ",s4));

s5=search(["a"],sTable4,1);
echo(str("Value vector WARNING ([\"a\"]): ",s5));

// Value searches
vTable1=[1,2,3];
v1 = search(3, vTable1);
echo(str("Default value search (3): ", v1));

vTable1=[1,2,3];
v2 = search(4, vTable1);
echo(str("Value not found (4): ", v2));
vTable2=[[0,1],[1,2],[2,3]];
v3 = search([1,2], vTable2);
echo(str("Value vector WARNING ([1,2]): ", v3));

// number searches
nTable1=[ [1,"a"],[3,"b"],[2,"c"],[4,"d"],[1,"a"],[7,"b"],[2,"c"],[8,"d"],[9,"e"],[10,"a"],[1,"a"] ];
n1 = search(7,nTable1);
echo(str("Default number search (7): ",n1));
n2 = search(1,nTable1);
echo(str("Return all matches for number search (1): ",n2));

// list searches
lTable1=[ [1,"a"],[3,"b"],[2,"c"],[4,"d"],[1,"a"],[7,"b"],[2,"c"],[8,"d"],[9,"e"],[10,"a"],[1,"a"] ];
lSearch1=[1,3,1000];
l1=search_vector(lSearch1,lTable1);
echo(str("Default list number user-defined search_vector function (",lSearch1,"): ",l1));

lTable2=[ ["cat",1],["b",2],["c",3],["dog",4],["a",5],["b",6],["c",7],["d",8],["e",9],["apple",10],["a",11] ];
lSearch2=["b","zzz","a","c","apple","dog"];
l2=search_vector(lSearch2,lTable2);
echo(str("Default list string user-defined search_vector function (",lSearch2,"): ",l2));

lTable3=[ ["cat",1],["b",2],["c",3],[4,"dog"],["a",5],["b",6],["c",7],["d",8],["e",9],["apple",10],["a",11] ];
lSearch3=["b",4,"zzz","c","apple",500,"a",""];
l3=search_vector(lSearch3,lTable3);
echo(str("Default list mixed user-defined search_vector function (",lSearch3,"): ",l3));

l4=search_vector(lSearch3,lTable3);
echo(str("Return all matches for mixed user-defined search_vector function (",lSearch3,"): ",l4));

lSearch5=[1,"zz","dog","a",500,11];
l5=search_vector(lSearch5,lTable3,1);
echo(str("Return all matches for mixed user-defined search_vector function; alternate columns (",lSearch5,"): ",l5));


// for completeness
cube(1.0);
