for(a = [-180:179])
    if(atan2(sin(a),cos(a)) - a)
        echo("atan2", a, atan2(sin(a),cos(a)) - a);

for(a = [-90:90])
    if(asin(sin(a)) - a)
        echo("asin", a, asin(sin(a)) - a);

for(a = [0:180])
    if(acos(cos(a)) - a)
        echo("acos", a, acos(cos(a)) - a);

for(a = [-89:89])
    if(atan(tan(a)) - a)
        echo("atan", a, atan(tan(a)) - a);
     