from openscad import *
from random import *

"""
 Lasercutter class which can be used to turn a 3D solid into lasercut assembling pieces
 Its based an the faces() interface
"""
class LaserCutter:

    def __init__(self, facelist, depth=4):
        self.init_sub(facelist, depth)

    def __init__(self, obj, depth=4, rows=4, cols=4):
        unitmatrix = rotx(cube().origin, 90)

        height=obj.size[2]

        xpitch=obj.size[0]/cols
        ypitch=obj.size[1]/rows
        pos=obj.position

        faces = []
        for r in range(rows):
            mat = translate(rotz(unitmatrix,0),[0,pos[1]+ypitch*(r+0.5),0])
            cut=obj.divmatrix(mat).projection(cut=True)
            faces.append(cut.multmatrix(mat))


        for c in range(cols):
            mat = translate(rotz(unitmatrix,90),[pos[0]+xpitch*(c+0.5),0,0])
            cut=obj.divmatrix(mat).projection(cut=True)
            faces.append(cut.multmatrix(mat))

        self.init_sub(faces, depth)
        print(len(faces))

    def init_sub(self, facelist, depth):

        self.depth=depth
        conn_plain = [[0,0],[1,0]]

                # x is realtive, y is absolute
        conn_type1 = [ [0.0,-1], [0.25,-1], [0.25,1], [0.5,1], [0.5,-1], [0.75,-1], [0.75, 1], [1,1]]
        conn_type2 = [ [0.0,-1], [0.4,-1], [0.4,1], [0.6,1], [0.6,-1], [1,-1] ]
        conn_type2_ = [[0.4,-1],[0.6 , -1] ,[0.6,1],[0.4,1]]
        conn_type3  = [[0,-1],[1, -1],[1,1],[0,1]]

        late_cuts = []
        total_cuts = []

        #LUT
        lut = {}
        vertices = []
        edge_inv = {}

        # make sure all faces are polygons
        for i,f in enumerate(facelist):
            pol, = f.children()
            points = []
            paths = []
            for pts in pol.mesh():
                s=len(points)
                n=len(pts)
                points = points + pts
                paths.append(list(range(s, s+n)))
            pol = polygon(points=points, paths = paths)
            facelist[i] = pol.multmatrix(f.matrix)

        # faces, outlines, segments, other
        ifaces = [] # indexed ifaces[face][pathnum][ptnum]
        for f in facelist:
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
        self.pieces = []
        for i, f in enumerate(facelist):
            mat = f.matrix
            pol, = f.children()
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
                        for l, oface in enumerate(facelist):
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


                for k, oface in enumerate(facelist):
                    if k == i:
                        continue
                    pt1x=divmatrix(pt1, oface.matrix)
                    pt2x=divmatrix(pt2, oface.matrix)
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
            piece = polygon(points=newpolpoints, paths=newpolpaths).multmatrix(mat)
            self.pieces.append(piece)

            #zvec = list(zip(*f.matrix))[2][:3] # Z vector
            #dann poly2  als 3d divmatrix mit plane1 z=0 punkte sammeln sortieren nach x oder y, 2er gruppen bilden
            #nur wenn 1 gruppe und auf kante liegt
            #n1 cross n2 z>0: oben  sonst uinten

        # create more late cuts from toal cuts
        # finished all faces
        for i, cuts in enumerate(total_cuts):
            f = facelist[i]

            fn = [f.matrix[0][2], f.matrix[1][2], f.matrix[2][2]]

            for j in range(int(len(cuts)/3)):
                p1=cuts[3*j+0]
                p2=cuts[3*j+1]

                oi=cuts[3*j+2]
                fo = facelist[oi]
                fno = [fo.matrix[0][2], fo.matrix[1][2], fo.matrix[2][2]]
                fc=cross(fn, fno)
                pmid = [(p1[i]+p2[i])/2.0 for i in range(2) ]
                # Strecke halbierem oben oder unten
                if cross(fn, fno)[2] > 0:
                    cutout = self.jigging(pmid,p1, conn_type3)
                else:
                    cutout = self.jigging(pmid,p2, conn_type3)
                late_cuts[i] = union(late_cuts[i], polygon(cutout))

        # Apply late cuts now
        newpieces = []
        for i, piece in enumerate(self.pieces):
            pol, = piece.children()
            newpol = pol - late_cuts[i]
            newpieces.append(multmatrix(newpol, piece.matrix))
        self.pieces = newpieces

    def jigging(self, beg, end, conn):
        diff = [end[0]-beg[0], end[1]-beg[1]]
        dl = norm(diff)
        stripe = []
        for jig in conn:
            pt = [
                    beg[0] + jig[0]*diff[0] + jig[1]*diff[1]*self.depth/2/dl,
                    beg[1] + jig[0]*diff[1] - jig[1]*diff[0]*self.depth/2/dl
            ]
            stripe.append(pt)
        return stripe


    def preview(self):
        for piece in self.pieces:
            col = [ (random()+1)*0.5 for i in range(3)]
            pol, = piece.children()
            pol.linear_extrude(height=self.depth).down(self.depth/2).multmatrix(piece.matrix).color(col).show()

    def collision(self):
        items=[]
        for piece in self.pieces:
            pol, = piece.children()
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


    def finalize(self,kerf=0.1, dist=1):
        # Calculate boundingbox
        puzzles = []
        for piece in self.pieces:
            puzzle, = piece.children()
            puzzle = puzzle.offset(kerf)
            puzzle.bbox = self.calc_bbox(puzzle)
            puzzles.append(puzzle)

        # combine all of them by abuting width/length
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
                        if score < bestscore:
                            bestscore= score
                            bestcomb = [0,i,j]
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
