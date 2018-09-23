a=undef;
b=undef;

echo("-- comparing undef --");
if(undef == undef){
    echo("undef is undef");
}

if(a == undef){
    echo("a is undef");
}

if(undef == a){
    echo("undef is a");
}

if(a == b){
    echo("a is b");
}

if(c == undef){
    echo("c is undef");
}

if(undef == c){
    echo("undef is c");
}

echo("-- comparing undef --");
if(a){
    echo("undef evaluates true");
}else{
    echo("undef evaluates false");
}

echo(str("undef evaluates ",c ? true:false));

echo("-- echo undef --");
echo(a);
echo(c);

echo("-- calculating with undef --");
echo(a/1);
echo(a/0);
echo(1/a);


echo("-- calculating resulting in +inf --");
echo(1/0);
echo(-1/-0);

echo("-- calculating resulting in -inf --");
echo(1/-0);
echo(-1/0);

d = 10/0;
e = -1/-0;

echo("-- comparing inf --");
if(d==e){
    echo("inf == inf");
}else{
    echo("inf != inf");
}

echo("-- 3d objects --");
cube(a);
sphere(e);
cylinder(r=-1/0,h=e);

echo("-- for loops --");
for(j = [a : b]){
    echo(j);
}

for(i = [-1/0 : 1/0]){
    echo(i);
}
