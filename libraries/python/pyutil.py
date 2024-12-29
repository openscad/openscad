from openscad import *
from math import *

def loft_prepare(solid1, solid2,n, rot ):
    loft_pts1=solid1.mesh()[0] # TODO fix
    loft_pts2=solid2.mesh()[0]
    loft_ang = []
    
    for pt in loft_pts1:
        ang=atan2(pt[1], pt[0])
        if ang not in loft_ang:
            loft_ang.append(ang)
    for pt in loft_pts2:
        ang=atan2(pt[1], pt[0])-rot
        if ang < -3.1415926:
            ang=ang+2*3.1415926
        if ang not in loft_ang:
            loft_ang.append(ang)
    for i in range(n):
         ang=3.14159265359*(2*i/n-1)
         if ang not in loft_ang:
             loft_ang.append(ang)               
    loft_ang.sort()
    
    loft_data= [loft_ang]
    rnd=0
    for set in  loft_pts1, loft_pts2:
        mags = []
        
        for ang in loft_ang:
            if rnd == 1:
                ang=ang+rot
                if ang > 3.1415926:
                    ang=ang-2*3.1415926
            v=[cos(ang),sin(ang)]
        
            oldpt=set[-1]
            # where does it cut
            mag=1
            for i in range(len(set)):
                tail=set[i]
                head=set[(i+1)%len(set)]
                d= [head[0]-tail[0],head[1]-tail[1]]
                
                det=d[1]*v[0]-d[0]*v[1]
                if det != 0:
                    a=(tail[0]*v[1] - tail[1]*v[0])/det
                    if a< 0 or a > 1:
                        continue
                    cut=[tail[0]+a*d[0], tail[1]+a*d[1]]
                    if v[0] != 0:
                        b=cut[0]/v[0]
                    else:
                        b=cut[1]/v[1]
                    if b < 0:
                        continue
                    mag=sqrt(cut[0]*cut[0] + cut[1] * cut[1])
            mags.append(mag)    
        rnd=rnd+1
        loft_data.append(mags) 
    return loft_data            
                
def loft_func(loft_data, loft_height, h, rot):
    f=h/loft_height
    pts=[]
    for i in range(len(loft_data[0])):
        ang=loft_data[0][i]+rot*f
        mag=loft_data[1][i]*(1-f)+loft_data[2][i]*f
        pts.append([mag*cos(ang), mag*sin(ang)])
    return pts

def loft(shape1, shape2,loft_height,n=20, rot=0):
    rot=rot*3.14/180.0
    loft_data = loft_prepare(shape1, shape2,n, rot)
    return lambda h: loft_func(loft_data, loft_height, h, rot)
    
