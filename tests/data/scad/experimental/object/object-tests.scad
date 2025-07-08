o1 = object(a=1,b=2,c=[3], f= function() 1);
all = object(t=true,i=42, d=42.42, v=[3], f= function() 1, o=object( a=1, b=2 ));

// printouts

echo("basic format              ",                  o1);
echo("basic format of a copy    ",                  object(o1));

// type functions

test( function() !is_undef(o1),                     "o1 must not be undef");
test( function() !is_bool(o1),                      "o1 must not be a bool");
test( function() !is_num(o1),                       "o1 must not be a num");
test( function() !is_string(o1),                    "o1 must not be a string");
test( function() !is_list(o1),                      "o1 must not be a list");
test( function() !is_function(o1),                  "o1 must not be a function");

// member access

test( function() o1["a"]==1,                        "o1['a'] must be 1");
test( function() o1["c"]==[3],                      "o1['c'] must be [3]");
test( function() o1.a==1,                           "o1.a' must be 1");
test( function() o1.c==[3],                         "o1.c must be [3]");
test(function() is_function(o1["f"]),               "o1['f'] must be a function");
test(function() is_function(o1.f),                  "o1.f must be a function");
test( function() [for (i=o1) has_key(o1, i)] 
    ==  [true, true, true, true],                   "all keys must be present");
test( function() len([for (i=o1) i]) == 4,          "check we have 4 keys");

// empty key

empty_key = object([["","empty-key"]]);

test( function() has_key(empty_key,""),             "check if empty key exists");
test( function() empty_key[""] == "empty-key",      "check if empty key has correct value");
test( function() object() == object(empty_key,[[""]]), "remove empty key");


// comprehension

testEq( [ for (i = o1 ) i ], ["a", "b", "c", "f" ], "expected different set of keys");

// equality 
testEq( o1, o1,                                     "same object must be equal");
testEq( all, all,                                   "same object with all types must be equal");
testEq( o1, object(o1),                             "copy must be equal");
testNEq( o1, object(o1,[["f"]]),                    "copy must not be equal when f removed");
testNEq( all, object(all,[["o"]]),                  "copy must not be equal when o removed for all types");

// editing

testEq( object(a=4, b=2, c=[3]), object(o1, [["f"], ["a",4]]),                      "!,a=4");
testEq( object(a=1, b=4, c=[3]), object(o1, [["f"], ["b",4]]),                      "!f,b=4");
testEq( object(a=1, b=2, c=[4]), object(o1, [["f"], ["c",[4]]]),                    "!f,c=[4]");
testEq( object(a=1, b=2, c=[3]), object(o1, [["f"]]),                               "!f");
testEq( object(a=4, b=2, c=[3],x=42), object(o1, [["f"], ["a",4]], x=42),           "!f,a=4,x=42");
testEq( object(a=1, b=4, c=[3],x=42), object(o1, [["f"], ["b",4]], x=42),           "!f,b=4,x=42");
testEq( object(a=1, b=2, c=[4],x=42), object(o1, [["f"], ["c",[4]]], x=42),         "!f,c=[4],x=42");
testEq( object(a=1, b=2, c=[4],x=undef), object(o1, [["f"], ["c",[4]]], x=undef),   "!f,c=[4],x=undef");
testEq( object(a=1, b=2, c=[3],x=undef), object(o1, [["f"]], [], [], [], x=undef),  "!f []... x=undef");
testEq( object(a=1, b=2, c=[3],x=undef), object(o1, [["f"]], [], [], [], x=undef),  "!f []... x=undef");


// large numbers, to check if we do not get really long times

os = [ for ( i = [0:1000] ) object( v = i )];
test(function() len(os) == 1001,                    "check size");
test(function() os[1000].v == 1000,                 "check value");

entries = [ for( i=[1:100000] ) [ str("_",i), i]];
o = object(entries);
test( function() o._99999 == 99999,                 "test 100k entries");

// do random access to 100k entries 100k times
access = [ for ( n = rands(1,100000,100000)) floor(n) ];
for ( i=access ) {
    key = str("_",i) ;
    value=o[ key ];
    assert(value == i);
}


module test( f, s ) {
    if ( !f()) {
        echo("FAIL:", f, s );
    } else
        echo("PASS:", s );            
}

module testEq( a, b, s) {
    if ( a != b) {
        echo("FAIL:", a, "==", b, s );
    } else
        echo("PASS:", s );            
}
module testNEq( a, b, s) {
    if ( a == b) {
        echo("FAIL:", a, "==", b, s );
    } else
        echo("PASS:", s );            
}
