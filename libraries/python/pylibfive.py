import libfive as lv
import math
#http://www.gradientspace.com/tutorials/category/g3sharp

def lv_coord():
        return lv.x(),lv.y(),lv.z()

def lv_clamp(c,low, high):
    return lv.min(lv.max(c,low),high)

def lv_lerp(a,b,t):
    if type(a) is list:
        return  lv_lerp(a[0],b[0],t), lv_lerp(a[1],b[1],t), lv_lerp(a[2],b[2],t)
    else:
        return  a*(1-t) + b*t

def lv_vecsub(a,b):
    return a[0]-b[0],a[1]-b[1],a[2]-b[2]

def lv_dot(a,b):
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]

def lv_length(c):
    return lv.sqrt(c[0]*c[0]+c[1]*c[1]+c[2]*c[2])

def lv_trans(c,v):
    return c[0]-v[0] ,c[1]-v[1],c[2]-v[2]

def lv_matrix_sub(c,p, n):
    return (lv_clamp(c+p/2,0,p*n)%p)-p/2

def lv_matrix(c,p, n):
    return lv_matrix_sub(c[0],p[0],n[0]), lv_matrix_sub(c[1],p[1],n[1]), lv_matrix_sub(c[2],p[2],n[2])

def lv_scalar(a,b):
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]

def lv_len(n):
    return lv.sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2])

def lv_vec_scale(v,n):
    return v[0]*n, v[1]*n, v[2]*n

def lv_vec_unit(v):
    return lv_vec_scale(v,1.0/lv_len(v))

def lv_rotxy(p,ang):
     s=math.cos(3.14*ang/180)
     c=math.sin(3.14*ang/180)
     return  p[1]*c-p[0]*s,p[0]*c+p[1]*s,p[2]

def lv_rotxz(p,ang):
     s=math.cos(3.14*ang/180)
     c=math.sin(3.14*ang/180)
     return  p[2]*c-p[0]*s,p[1],p[0]*c+p[2]*s

def lv_rotyz(p,ang):
     s=math.cos(3.14*ang/180)
     c=math.sin(3.14*ang/180)
     return  p[0],p[2]*c-p[1]*sp[1]*c+p[2]*s

def lv_mirror(c,n1):
    n=lv_vec_unit(n1)
    e =lv_scalar(c,n)
    x=n*(lv.abs(e)-e)
    return c[0]+x[0],c[1]+x[1],c[2]+x[2]

def lv_cubemiror(c, func):
    c1=c[0],c[1],lv.abs(c[2])
    c2=c[1],c[2],lv.abs(c[0])
    c3=c[2],c[0],lv.abs(c[1])
    return lv.min(lv.min(func(c1),func(c2)),func(c3))

def lv_octmirror(c,func):
    r = None
    for j in range(2):
        for i in range(4):
            rx=func(lv_rotxz(lv_rotxy(c,i*90),54+72*j))
            if r == None:
                r=rx
            else:
                r=lv.min(r,rx)
    return r

def lv_dodmirror(c, func):
   r=func([c[0],c[1],lv.abs(c[2])])
   for w in range(5):
       cs=lv_rotxz(lv_rotxy(c,w*72),63.44)
       csx=cs[0],cs[1],lv.abs(cs[2])
       r=lv.min(r,func(csx))
   return r

def lv_sphere(c,r):
    return lv_length(c)-r 

def lv_box(c, box):
	q= lv.abs(c[0])-box[0], lv.abs(c[1])-box[1], lv.abs(c[2])-box[2]
	return lv_length( [lv.max(q[0],0), lv.max(q[1],0), lv.max(q[2],0)] )+ lv.min(lv.max(lv.max(q[0],q[1]),q[2]),0)

def lv_cylinder(c, h,r1,r2=-1):
    if r2 == -1:
        r2=r1
    r=lv_lerp(r1,r2,(c[2]/h))
    distlat = lv_length([c[0],c[1],0]) - r
    distvert = lv.max(c[2]-h,-c[2])
    return lv_length([ lv.max(distlat,0), 0, lv.max(distvert,0)])+ lv.min(lv.max(distlat,distvert),0)

def lv_cylinder_c(c, h,r):
    distlat= lv.abs(xr)-r
    distvert=lv.max(c[2]-h,-c[2])
    return lv_length([ lv.max(distlat,0), 0, lv.max(distvert,0)])+ lv.min(lv.max(distlat,distvert),0)

# https://www.youtube.com/watch?v=-pdSjBPH3zM    

def lv_segment(c, a, b):
    v1=lv_vecsub(c,a)
    v2=lv_vecsub(b,a)
    n=lv_clamp(lv_dot(v1,v2)/lv.square(lv_length(v2)),0,1)
    d=lv_vecsub(c,lv_lerp(a,b,n))
    return  lv_length(d)

def lv_union(a, b):
    return lv.min(a, b)

def lv_union_chamfer(a, b,r): # Credit: Doug Moen
    e = lv.max(r - lv.abs(a - b), 0);
    return lv.min( a, b) - e*0.5

def lv_union_ring(a, b,r):
    return lv.min(lv.min(a, b),lv.sqrt(a*a+b*b)-r)

def lv_union_groove(a, b,r):
    x=lv_clamp(r-lv.sqrt(a*a+b*b),0,r)
    return lv.min(a+x, b+x)

def lv_union_smooth(a, b, r): #Credit: original algorithm in GLSL by Inigo Quilez

    h = lv_clamp (((b-a)/r+1)*0.5, 0, 1)
    return lv_lerp(b, a, h) + r*h*(h-1)

def lv_union_stairs(a,b,r,n): # Credit: original algorithm in GLSL by @paniq
    s = r/(n+1.0)
    u = b-r;
    return lv.min( lv.min(a,b), ( a+u+lv.abs( (-a+u+s)%(2*s) - s))*0.5)


def lv_intersection(a, b):
    return -lv_union(-a,-b) 

def lv_intersection_chamfer(a, b,r):
    return -lv_union_chamfer(-a,-b,r)

def lv_intersection_ring(a, b,r):
    return -lv_union_ring(-a,-b,r)

def lv_intersection_groove(a, b,r):
    return -lv_union_groove(-a,-b,r)

def lv_intersection_smooth(a, b,r):
    return -lv_union_smooth(-a,-b,r)

def lv_intersection_stairs(a, b,r,n):
    return -lv_union_stairs(-a,-b,r,n)

def lv_difference(a, b):
    return -lv_union(-a,b) 

def lv_difference_chamfer(a, b,r):
    return -lv_union_chamfer(-a,b,r)

def lv_difference_ring(a, b,r):
    return -lv_union_ring(-a,b,r)

def lv_difference_groove(a, b,r):
    return -lv_union_groove(-a,b,r)

def lv_difference_smooth(a, b,r):
    return -lv_union_smooth(-a,b,r)

def lv_difference_stairs(a, b,r,n):
    return -lv_union_stairs(-a,b,r,n)

def lv_juntion_ring(a,b,r):
    return lv.sqrt(a*a+b*b)-r

         
