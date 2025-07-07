o1 = object(a=1,b=2,c=[3], f= function() 1);
echo("warnings");
echo( object(o1, [[true,5]]), "true as set key");
echo( object(o1, [[undef]]), "undef as del key");
echo( object(o1, undef), "undef as edit list");
echo( object(o1, [undef]), "undef as entry in edit list");
echo( object(o1, [[""],undef]), "undef 2nd entry as edit list");
