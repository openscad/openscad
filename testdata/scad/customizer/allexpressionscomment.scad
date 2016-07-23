a = true;//parameter
//description
b = false;//parameter
//description
c = undef;//parameter
//description
d = a;//parameter
//description
e = $fn;//parameter
//description
f1 = [1,,];//parameter
//description
f2 = [1,2,3];//parameter
//description
g = f2.x + f2.y + f2.z;//parameter
//description
h1 = [2:5];//parameter
//description
h2 = [1:2:10];//parameter
//description
i = h2.begin - h2.step - h2.end;//parameter
//description
j = "test";//parameter
//description
k = 1.23e-2;//parameter
//description
l = a * b;//parameter
//description
m = a / b;//parameter
//description
n = a % b;//parameter
//description
o = c < d;//parameter
//description
p = c <= d;//parameter
//description
q = c == d;//parameter
//description
r = c != d;//parameter
//description
s = c >= d;//parameter
//description
t = c > d;//parameter
//description
u = e && g;//parameter
//description
v = e || g;//parameter
//description
w = +i;//parameter
//description
x = -i;//parameter
//description
y = !i;//parameter
//description
z = (j);//parameter
//description
aa = k ? l : m;//parameter
//description
bb = n[o];//parameter
//description
cc = let(a=1) a;//parameter
//description
dd = [for (a=[0,1]) let(b=a) if (true) b];//parameter
//description
ee = ["abc", for (a=[0,1]) let(b=a) if (true) b, true, for(c=[1:3]) c, 3];//parameter
//description
ff = [for (a=[0,1]) if (a == 0) "A" else ( "B" )];//parameter
//description
gg = [each [ "a", 0, false ]];//parameter
//description
hh = [for (a = [0 : 3]) if (a < 2) ( if (a < 1) ["+", a] ) else ["-", a] ];//parameter
//description
ii = [for (a=0,b=1;a < 5;a=a+1,b=b+2) [a,b*b] ];
