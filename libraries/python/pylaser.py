from openscad import *
from random import *

"""
 Lasercutter class which can be used to turn a 3D solid into lasercut assembling pieces
 Its based an the faces() interface
"""
class LaserCutter:

    def __init__(self, depth=4, rows=4, cols=4):
        self.depth=depth
        self.faces = []

    def add_surface(self,obj):
        self.faces = self.faces + obj.faces()

    def add_volume(self,obj, rows=4, cols=4):
        unitmatrix = rotx(cube().origin, 90)

        height=obj.size[2]

        xpitch=obj.size[0]/cols
        ypitch=obj.size[1]/rows
        pos=obj.position

        for r in range(rows):
            mat = translate(rotz(unitmatrix,0),[0,pos[1]+ypitch*(r+0.5),0])
            cut=obj.divmatrix(mat).projection(cut=True)
            self.faces.append(cut.multmatrix(mat))


        for c in range(cols):
            mat = translate(rotz(unitmatrix,90),[pos[0]+xpitch*(c+0.5),0,0])
            cut=obj.divmatrix(mat).projection(cut=True)
            self.faces.append(cut.multmatrix(mat))

    def add_volume_tri(self,obj, cols=4):
        unitmatrix = rotx(cube().origin, 90)

        height=obj.size[2]

        pitch=obj.size[0]/cols
        pos=obj.position
        center=[obj.position[i] + obj.size[i]/2 for i in range(3)]
        diffangle=60

        for r in range(3):
            for i in range(cols):
                mat = translate(rotz(translate(unitmatrix,[0,(2*i-cols+1)*pitch/2,0]),r*diffangle),center)
                obj.divmatrix(mat)
                cut=obj.divmatrix(mat).projection(cut=True)
                face=cut.multmatrix(mat)
                face.stack = [r,3]
                self.faces.append(face)



    def add_faces(self, facelist):
        self.faces = self.faces + facelist

    def link(self):

        conn_plain = [[0,0, 0],[1,0, 0]]

        # [x realtive, y is absolute, x absolute
        conn_type1 = [ [0.0,-1, 0], [0.25,-1, 0], [0.25,1, 0], [0.5,1, 0], [0.5,-1, 0], [0.75,-1, 0], [0.75, 1, 0], [1,1, 0]]
        conn_type2 = [ [0.0,-1, 0], [0.4,-1, 0], [0.4,1, 0], [0.6,1, 0], [0.6,-1, 0], [1,-1, 0] ]
        conn_type2_ = [[0.4,-1, 0],[0.6 , -1, 0] ,[0.6,1, 0],[0.4,1, 0]]
        conn_type3  = [[0,-1, 0],[1, -1,0],[1,1,0],[0,1,0]]

        late_cuts = []
        total_cuts = []

        #LUT
        lut = {}
        vertices = []
        edge_inv = {}

        # make sure all faces are polygons
        for f in self.faces:
            for pol in f:
                points = []
                paths = []
                for pts in pol.mesh():
                    s=len(points)
                    n=len(pts)
                    points = points + pts
                    paths.append(list(range(s, s+n)))
                pol.value  = polygon(points=points, paths = paths)


        # faces, outlines, segments, other
        ifaces = [] # indexed ifaces[face][pathnum][ptnum]
        for f in self.faces:
            late_cuts.append(False) # dummy void
            total_cuts.append([])
            mat = f.matrix
            pol, = f.children()

            #pt inventur
            ipaths=[]
            if len(pol.paths) == 0:
                pol.paths=[list(range(len(pol.points)))]
            for path in pol.paths:

                ipath = []

                for ind in path:
                    pt = pol.points[ind]
                    pt3 = multmatrix(pt + [0] , mat)
                    # check if pt3 is very similar to an exising one
                    for x, v in enumerate(vertices):
                        if abs(v[0] - pt3[0]) < 1e-3 and abs(v[1] - pt3[1] ) < 1e-3 and  abs(v[2] - pt3[2]) < 1e-3:
                            pt3=v
                            lut[tuple(pt3)]=x
                            break
                    pt3t = tuple(pt3)
                    # Create LUT from vertices
                    if not pt3t in lut:
                        ind=len(vertices)
                        lut[pt3t]=ind
                        vertices.append(pt3)
                    ipath.append(lut[pt3t])

                    # Create Eedge inventory
                ipaths.append(ipath)
                n=len(ipath)
                for i in range(n):
                    edge=[ipath[i],ipath[(i+1)%n]]
                    edge_inv[tuple(edge)]=1
            ifaces.append(ipaths)

        # Process faces and edges now

        for i, f in enumerate(self.faces):
            mat = f.matrix
            for pol in f:
                if len(pol.paths) == 0:
                    pol.paths=[list(range(len(pol.points)))]
                newpolpoints=[]
                newpolpaths=[]
                totalcuts=[]
                for j, path in enumerate(pol.paths):
                    newpts=[]
                    curface=ifaces[i][j]
    
                    n=len(curface)
    
                    stripes = [] # pattern on each side
    
                    for k in range(n): # all edges
                        conn=conn_plain
                        partedge=[curface[(k+1)%n],curface[k]]
                        if tuple(partedge) in edge_inv:
                            conn = conn_type1
    
                        if conn == conn_plain:
                            pt1=multmatrix(pol.points[path[k]] + [0], mat)
                            pt2=multmatrix(pol.points[path[(k+1)%n]] + [0], mat) # TODO hier out of index
    
                            # go through all faces and check if they are inside
                            for l, oface in enumerate(self.faces):
                                if l == i:
                                    continue
                                # pos auf oface
                                pt1x=divmatrix(pt1, oface.matrix)
                                pt2x=divmatrix(pt2, oface.matrix)
                                oshape,  = oface.children()
                                valid=True
                                if abs(pt1x[2]) > 1e-3:
                                    valid = False
                                if abs(pt2x[2]) > 1e-3:
                                    valid = False
                                if not oface.inside(pt1x):
                                    valid = False
                                if not oface.inside(pt2x):
                                    valid = False
    
                                if valid == True:
                                    conn = conn_type2
                                    cutout = self.jigging(pt1x, pt2x, conn_type2_)
                                    late_cuts[l] = union(late_cuts[l], polygon(cutout))

                        beg = pol.points[path[k]]
                        end = pol.points[path[(k+1)%n]]
                        stripe = self.jigging(beg,end, conn)
                        stripes.append(stripe)


                    for k, oface in enumerate(self.faces):
                        if k == i:
                            continue
                        oshape,  = oface.children()
    
                        for l in range(n):
    
                            pt1=multmatrix(pol.points[path[l]] + [0], mat)
                            pt2=multmatrix(pol.points[path[(l+1)%n]] + [0], mat) # TODO hier out of index
                            pt1x=divmatrix(pt1, oface.matrix)
                            pt2x=divmatrix(pt2, oface.matrix)
    
                            fact=None
                            if pt1x[2] < -1e-3 and pt2x[2] > 1e-3:
                                # calculate exact cut
                                fact=pt1x[2]/(pt1x[2]-pt2x[2])
        
                            if pt2x[2] < -1e-3 and pt1x[2] > 1e-3:
                                fact= pt1x[2]/(pt1x[2]-pt2x[2])
                            if fact is not None:
                                ptcut=[ pt1x[i]+(pt2x[i]-pt1x[i])*fact for i in range(2) ]
                                total_cuts[k].append(ptcut)
                            if len(total_cuts[k])%3 == 2:
                                total_cuts[k].append(i) # 2->3, info about other plate



                    #finished all edges, alle
                    # Now concatenate all strips with extrapolation
                    for k, stripe in enumerate(stripes):
                        nextstr = stripes[(k+1)%n]
    
                        # middle part of edge
                        newpts = newpts + stripe[1:-1] # skip 1st and last
                        #calculate joint point
                        p1=stripe[-2]
                        p2=stripe[-1]
                        p3=nextstr[0]
                        p4=nextstr[1]
                        x21=p2[0]-p1[0]
                        y21=p2[1]-p1[1]
                        x31=p3[0]-p1[0]
                        y31=p3[1]-p1[1]
                        x43=p4[0]-p3[0]
                        y43=p4[1]-p3[1]
                        s=(x43*y31-x31*y43)/(x43*y21-x21*y43)
                        pt = [p1[0]+ s*x21, p1[1] + s*y21]
                        newpts.append(pt)
    
                    s=len(newpolpoints)
                    n=len(newpts)
                    newpolpoints = newpolpoints + newpts
                    newpolpaths.append(list(range(s, s+n)))
                # finished all outlines
                pol.value = polygon(points=newpolpoints, paths=newpolpaths)

            #zvec = list(zip(*f.matrix))[2][:3] # Z vector
            #dann poly2  als 3d divmatrix mit plane1 z=0 punkte sammeln sortieren nach x oder y, 2er gruppen bilden
            #nur wenn 1 gruppe und auf kante liegt
            #n1 cross n2 z>0: oben  sonst uinten

        # create more late cuts from total cuts
        # finished all faces
        for i, cuts in enumerate(total_cuts):
            f = self.faces[i]

            fn = [f.matrix[0][2], f.matrix[1][2], f.matrix[2][2]]

            for j in range(int(len(cuts)/3)):
                p1=cuts[3*j+0]
                p2=cuts[3*j+1]
                oi=cuts[3*j+2]
                fo = self.faces[oi]
                fno = [fo.matrix[0][2], fo.matrix[1][2], fo.matrix[2][2]]
                if self.faces[i].hasattr("stack"):
                    m=self.faces[i].stack[0]
                    n=self.faces[i].stack[1]
                    wf=1.0/Tan(90/n)
                    conn  = [[0,-wf, 0],[1, -wf,0],[1,wf,0],[0,wf,0]]
                    if m > 0:
                        #lower cut 0-m
                        pmid = [(p2[i]*m+p1[i]*(n-m))/n for i in range(2) ]
                        cutout = self.jigging(pmid,p1, conn)
                        late_cuts[i] = union(late_cuts[i], polygon(cutout))
                    if m < n-1:
                        # upper cut m+1-n
                        pmid = [(p2[i]*(m+1)+p1[i]*(n-m-1))/n for i in range(2) ]
                        cutout = self.jigging(pmid,p2, conn)
                        late_cuts[i] = union(late_cuts[i], polygon(cutout))
                        # make gap thicker by tan(phi/2)
                        pass
                else:
                    # cross regelung
                    fc=cross(fn, fno)
                    pmid = [(p1[i]+p2[i])/2.0 for i in range(2) ]
                    # Strecke halbierem oben oder unten
                    if cross(fn, fno)[2] > 0:
                        cutout = self.jigging(pmid,p1, conn_type3)
                    else:
                        cutout = self.jigging(pmid,p2, conn_type3)
                    late_cuts[i] = union(late_cuts[i], polygon(cutout))

        # Apply late cuts now
        for i, piece in enumerate(self.faces):
            for pol in piece:
                if late_cuts[i] != False:
                    pol.value = pol.value - late_cuts[i]

    def jigging(self, beg, end, conn):
        diff = [end[0]-beg[0], end[1]-beg[1]]
        dl = norm(diff)
        stripe = []
        for jig in conn:
            pt = [
                    beg[0] + jig[0]*diff[0] + jig[1]*diff[1]*self.depth/2/dl + jig[2]*diff[0]/dl,
                    beg[1] + jig[0]*diff[1] - jig[1]*diff[0]*self.depth/2/dl + jig[2]*diff[1]/dl
            ]
            stripe.append(pt)
        return stripe


    def preview(self):
        for piece in self.faces:
            col = [ (random()+1)*0.5 for i in range(3)]
            for pol in piece:
                pol.linear_extrude(height=self.depth).down(self.depth/2).multmatrix(piece.matrix).color(col).show()

    def collision(self):
        items=[]
        for piece in self.faces:
            for pol in piece:
                item = pol.offset(-0.01).linear_extrude(height=self.depth).down(self.depth/2).multmatrix(piece.matrix)
                items.append(item)
        n=len(items)
        for i in range(n):
            for j in range(n):
                if j > i:
                    intersection(items[i], items[j]).show()

    def calc_bbox(self, puzzle):
        m = puzzle.mesh()
        xmin=None
        ymin=None
        xmax=None
        ymax=None
        for out in m:
            for pt in out:
                if xmin == None or pt[0] < xmin:
                    xmin=pt[0]
                if ymin == None or pt[1] < ymin:
                    ymin=pt[1]
                if xmax == None or pt[0] > xmax:
                    xmax=pt[0]
                if ymax == None or pt[1] > ymax:
                    ymax=pt[1]

        return [xmin, ymin, xmax, ymax]


 
        

    def finalize_auto(self, puzzles):
        done=True
        while done == True:
            n=len(puzzles)
            done=False

            # find out best mach

            bestscore=1e6 # verschwendete flaeche
            bestcomb = None
            for i in range(n):

                wid_i = puzzles[i].bbox[2] - puzzles[i].bbox[0]
                hei_i = puzzles[i].bbox[3] - puzzles[i].bbox[1]
                for j in range(n):
                    if j > i:
                        wid_j = puzzles[j].bbox[2] - puzzles[j].bbox[0]
                        hei_j = puzzles[j].bbox[3] - puzzles[j].bbox[1]
                        score=max(wid_j,wid_i)*(hei_i+hei_j)-wid_i*hei_i - wid_j*hei_j
                        if maxheight == None or hei_i+ hei_j < maxheight:
                            if score < bestscore :
                                bestscore= score
                                bestcomb = [0,i,j]
                        if maxwidth == None or wid_i + wid_j < maxwidth:
                            score=max(hei_j,hei_i)*(wid_i+wid_j)-wid_i*hei_i - wid_j*hei_j
                            if score < bestscore:
                                bestscore= score
                                bestcomb = [1,i,j]




            if bestcomb is not None:
                i=bestcomb[1]
                j=bestcomb[2]
                wid_i = puzzles[i].bbox[2] - puzzles[i].bbox[0]
                hei_i = puzzles[i].bbox[3] - puzzles[i].bbox[1]
                wid_j = puzzles[j].bbox[2] - puzzles[j].bbox[0]
                hei_j = puzzles[j].bbox[3] - puzzles[j].bbox[1]
                if bestcomb[0] == 0: # vertical match

                    pieces = [puzzles[i], puzzles[j]]
                    comb = puzzles[i]
                    comb |= puzzles[j] + [
                                       puzzles[i].bbox[0] - puzzles[j].bbox[0],
                                       puzzles[i].bbox[3] - puzzles[j].bbox[1]+dist]

                    comb.bbox = self.calc_bbox(comb)
                    puzzles[i]  = comb

                    del puzzles[j]
                    done=True
                if bestcomb[0] == 1: # horizontal match

                    comb = puzzles[i]
                    comb |= puzzles[j] + [
                                       puzzles[i].bbox[2] - puzzles[j].bbox[0]+dist,
                                       puzzles[i].bbox[1] - puzzles[j].bbox[1]]

                    comb.bbox = self.calc_bbox(comb)
                    puzzles[i]  = comb

                    del puzzles[j]
                    done=True
        return puzzles[0]

    def finalize_combine_vert(self, pieces, dist):
        if len(pieces) == 0:
            return None
        comb=pieces[0]

        ref_x = pieces[0].bbox[0]
        ref_y = pieces[0].bbox[3]
        for piece in pieces[1:]:
            comb |= piece + [ ref_x - piece.bbox[0], ref_y - piece.bbox[1]+dist]
            ref_y = ref_y + piece.bbox[3]-piece.bbox[1]+dist
            
        comb.bbox = self.calc_bbox(comb)
        return comb

    def finalize_combine_hor(self, pieces, dist):
        if len(pieces) == 0:
            return None
        comb=pieces[0]

        ref_x = pieces[0].bbox[2]
        ref_y = pieces[0].bbox[1]
        for piece in pieces[1:]:
            comb |= piece + [ ref_x - piece.bbox[0]+dist, ref_y - piece.bbox[1]]
            ref_x = ref_x + piece.bbox[2]-piece.bbox[0]+dist
            
        comb.bbox = self.calc_bbox(comb)
        return comb


    def finalize_sub(self, puzzles, recipe,dist):
        if isinstance(recipe, list):
            pieces = [self.finalize_sub(puzzles, term, dist) for term in recipe]
            return self.finalize_combine_vert(pieces,dist)

        if isinstance(recipe, tuple):
            pieces = [self.finalize_sub(puzzles, term, dist) for term in recipe]
            return self.finalize_combine_hor(pieces,dist)

        if isinstance(recipe, int):
            if recipe >= 0 and recipe  < len(puzzles):
                return puzzles[recipe]
        return None

    def finalize(self,recipe = None, kerf=0.1, dist=1, maxwidth=None, maxheight=None):
        # Calculate boundingbox
        puzzles = []
        for piece in self.faces:
            for puzzle in piece:
                puzzle = puzzle.offset(kerf)
                puzzle.bbox = self.calc_bbox(puzzle)
                print("size %f %f\n"%(puzzle.bbox[2]-puzzle.bbox[0], puzzle.bbox[3]-puzzle.bbox[1]))
                puzzles.append(puzzle)

        # combine all of them by abuting width/length
        if recipe != None:
            return self.finalize_sub(puzzles, recipe, dist)

        return self.finalize_auto(puzzles)


    def alter_face(self, dir, alterfunc):
        def maxfunc(i):
            m=self.faces[i].matrix
            return m[0][3]*dir[0]+m[1][3]*dir[1]+m[2][3]*dir[2]
        ind=max(list(range(len(self.faces))), key=maxfunc)
        shape,=self.faces[ind].children()    
        newshape=alterfunc(shape)       
        if newshape == False:
            del self.faces[ind]
            return
        self.faces[ind]=newshape.multmatrix(self.faces[ind].matrix)

