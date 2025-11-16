// A vector with length 4 can be swizzled using rgba or xyzw swizzle indices.
v = [10,20,30,40];
echo(v.x);
echo(v.y);
echo(v.z);
echo(v.w);
echo(v.wy);
echo(v.zwy);
echo(v.xyzw);
echo(v.xyxy);

echo(v.r);
echo(v.g);
echo(v.b);
echo(v.a);
echo(v.ag);
echo(v.bag);
echo(v.rgba);
echo(v.rgrg);

// Indices between the xyzw and rgba sets can be mixed 
echo(v.xr);

// For vectors of length < 4, indices out of range will return undef
v1 = [1];
v2 = [1,2];
v3 = [1,2,3];
echo(v1.y);
echo(v1.g);
echo(v2.z);
echo(v2.b);
echo(v3.w);
echo(v3.a);

// A vector with length > 4 can be swizzled, but only the first 4 elements can be accessed
v5 = [11,22,33,44,55];
echo(v5.xyzw);
echo(v5.rgba);

// Swizzled vectors can have up to 4 elements; specifying more than 4 elements will return undef
echo(v.rrrr);
echo(v.rrrrr);
echo(v.xyxyx);

// As opposed to objects, indices have to be numbers. Indexing with e.g. ["r"] will return undef.
echo(v.r);
echo(v[0]);
echo(v["r"]);

