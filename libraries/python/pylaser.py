from openscad import *
from random import *

"""
 Lasercutter class which can be used to turn a 3D solid into lasercut assembling pieces
 Its based an the faces() interface
"""
class LaserCutter:
    def __init__(self, facelist, depth=5):

        self.depth=depth
        conn_plain = [[1,0]]
        conn_type1 = [ # x is realtive, y is absolute
            [0.0,-1],
            [0.25,-1],
            [0.25,1],
            [0.5,1],
            [0.5,-1],
            [0.75,-1],
            [0.75, 1],
            [1,1]]

        #LUT
        lut = {}
        vertices = []

        edge_inv = {}
        for f in facelist:
            mat = f.matrix
            pol, = f.children()
            #pt inventur
            for path in pol.paths:

                newind = []

                for ind in path:
                    pt = pol.points[ind]
                    pt3 = tuple(multmatrix(pt + [0] , mat))
                    # Create LUT from vertices
                    if not pt3 in lut:
                        ind=len(vertices)
                        lut[pt3]=ind
                        vertices.append(pt3)
                    newind.append(lut[pt3])
                    # Create Eedge inventory
                n=len(newind)
                for i in range(n):
                    edge=[newind[i],newind[(i+1)%n]]
                    edge_inv[tuple(edge)]=1


        # Process faces and edges now
        self.pieces = []
        for f in facelist:
            mat = f.matrix
            pol, = f.children()
            for path in pol.paths:
                newind = []
                for ind in path:
                    pt = pol.points[ind]
                    pt3 = tuple(multmatrix(pt + [0] , mat))
                    newind.append(lut[pt3])

                n=len(newind)

                stripes = []
                for i in range(n):
                    conn=conn_plain
                    partedge=[newind[(i+1)%n],newind[i]]
                    if tuple(partedge) in edge_inv:
                        print("has part")
                        conn = conn_type1
                    else:
                        print("noo poart")

                    beg = pol.points[path[i]]
                    end = pol.points[path[(i+1)%n]]
                    diff = [end[0]-beg[0], end[1]-beg[1]]
                    dl = norm(diff)
                    stripe = []
                    for jig in conn:
                        pt = [
                                beg[0] + jig[0]*diff[0] + jig[1]*diff[1]*self.depth/2/dl,
                                beg[1] + jig[0]*diff[1] - jig[1]*diff[0]*self.depth/2/dl
                        ]
                        stripe.append(pt)
                    stripes.append(stripe)


                # Now concatenate all srtrips with extrapolation
                newpts=[]
                for i, stripe in enumerate(stripes):
                    nextstr = stripes[(i+1)%n]

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



                piece = polygon(newpts).multmatrix(mat)
                self.pieces.append(piece)


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


    def finalize(self):
        # Calculate boundingbox
        puzzles = []
        for piece in self.pieces:
            puzzle, = piece.children()
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

            print(bestcomb)



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
                                       puzzles[i].bbox[3] - puzzles[j].bbox[1]+1]

                    comb.bbox = self.calc_bbox(comb)
                    puzzles[i]  = comb

                    del puzzles[j]
                    done=True
                if bestcomb[0] == 1: # horizontal match

                    comb = puzzles[i]
                    comb |= puzzles[j] + [
                                       puzzles[i].bbox[2] - puzzles[j].bbox[0]+1,
                                       puzzles[i].bbox[1] - puzzles[j].bbox[1]]

                    comb.bbox = self.calc_bbox(comb)
                    puzzles[i]  = comb

                    del puzzles[j]
                    done=True
        print("res",len(puzzles))
        puzzles[0].show()
