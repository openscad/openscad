/*                         D X F - G . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2019 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file dxf-g.c
 *
 * Program to convert from the DXF file format to BRL-CAD format
 *
 */

/* FIXME: most funcs should have 'const char*' instead of 'char *' in their args. */

// #include "common.h"

/* system headers */
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <list>
#include <vector>
#include <string>
#include <cmath>
#include <vector>
#include <set>

#include "./dxf.h"

/* replicated code from defines.h to avoid dependency */
#define SQRT_SMALL_FASTF    1.0e-39 /* This squared gives zero */

/* replicated code from vmath.h to avoid dependency */
#define X       0
#define Y       1
#define Z       2
#define H       3
#define W       H
#define M_PI  3.14159265358979323846
#define M_2PI 6.28318530717958647692
#define MSX     0
#define MSY     5
#define MSZ     10
#define MSA     15
#define MDX     3
#define MDY     7
#define MDZ     11
#define VSETALL(a,s)    { (a)[X] = (a)[Y] = (a)[Z] = (s); }
#define V_MIN(r,s)      if( (s) < (r) ) r = (s)
#define V_MAX(r,s)      if( (s) > (r) ) r = (s)
#define VMOVE(a,b)      { \
                        (a)[X] = (b)[X];\
                        (a)[Y] = (b)[Y];\
                        (a)[Z] = (b)[Z]; }
#define VJOIN2(a,b,c,d,e,f)     { \
         (a)[X] = (b)[X] + (c) * (d)[X] + (e) * (f)[X];\
         (a)[Y] = (b)[Y] + (c) * (d)[Y] + (e) * (f)[Y];\
         (a)[Z] = (b)[Z] + (c) * (d)[Z] + (e) * (f)[Z]; }
#ifdef SHORT_VECTORS
#define MAT4X3PNT(o,m,i) \
        { register double _f; \
        register int _i_m4x3p, _j_m4x3p; \
        _f = 0.0; \
        for(_j_m4x3p = 0; _j_m4x3p < 3; _j_m4x3p++)  \
                _f += (m)[_j_m4x3p+12] * (i)[_j_m4x3p]; \
        _f = 1.0/(_f + (m)[15]); \
        for(_i_m4x3p = 0; _i_m4x3p < 3; _i_m4x3p++) \
                (o)[_i_m4x3p] = 0.0; \
        for(_i_m4x3p = 0; _i_m4x3p < 3; _i_m4x3p++)  { \
                for(_j_m4x3p = 0; _j_m4x3p < 3; _j_m4x3p++) \
                        (o)[_i_m4x3p] += (m)[_j_m4x3p+4*_i_m4x3p] * (i)[_j_m4x3p]; \
        } \
        for(_i_m4x3p = 0; _i_m4x3p < 3; _i_m4x3p++)  { \
                (o)[_i_m4x3p] = ((o)[_i_m4x3p] + (m)[4*_i_m4x3p+3]) * _f; \
        } }
#else
#define MAT4X3PNT(o,m,i) \
        { register double _f; \
        _f = 1.0/((m)[12]*(i)[X] + (m)[13]*(i)[Y] + (m)[14]*(i)[Z] + (m)[15]);\
        (o)[X]=((m)[0]*(i)[X] + (m)[1]*(i)[Y] + (m)[ 2]*(i)[Z] + (m)[3]) * _f;\
        (o)[Y]=((m)[4]*(i)[X] + (m)[5]*(i)[Y] + (m)[ 6]*(i)[Z] + (m)[7]) * _f;\
        (o)[Z]=((m)[8]*(i)[X] + (m)[9]*(i)[Y] + (m)[10]*(i)[Z] + (m)[11])* _f;}
#endif /* SHORT_VECTORS */
#define MAT_IDN(m)      {\
        (m)[1] = (m)[2] = (m)[3] = (m)[4] =\
        (m)[6] = (m)[7] = (m)[8] = (m)[9] = \
        (m)[11] = (m)[12] = (m)[13] = (m)[14] = 0.0;\
        (m)[0] = (m)[5] = (m)[10] = (m)[15] = 1.0;}
#define MAT_SCALE_VEC(_m, _v) {\
        (_m)[MSX] = (_v)[X]; \
        (_m)[MSY] = (_v)[Y]; \
        (_m)[MSZ] = (_v)[Z]; }
#define MAT_DELTAS(m,x,y,z)     { \
				        (m)[MDX] = (x); \
				        (m)[MDY] = (y); \
				        (m)[MDZ] = (z); }
#define MAT_DELTAS_VEC(_m,_v)   \
                        MAT_DELTAS(_m, (_v)[X], (_v)[Y], (_v)[Z] )
#define MAGSQ(a)  ( (a)[X]*(a)[X] + (a)[Y]*(a)[Y] + (a)[Z]*(a)[Z] )
#define NEAR_ZERO(val,epsilon)  ( ((val) > -epsilon) && ((val) < epsilon) )
#define ZERO(val) NEAR_ZERO((val), SMALL_FASTF)
#define SMALL_FASTF           1.0e-77 /* Anything smaller is zero */
#define VCROSS(a,b,c)   { \
                        (a)[X] = (b)[Y] * (c)[Z] - (b)[Z] * (c)[Y];\
                        (a)[Y] = (b)[Z] * (c)[X] - (b)[X] * (c)[Z];\
                        (a)[Z] = (b)[X] * (c)[Y] - (b)[Y] * (c)[X]; }
#define MAGNITUDE(a)    sqrt( MAGSQ( a ) )
#ifdef vax
#  define VDIVIDE_TOL   ( 1.0e-10 )
#  define VUNITIZE_TOL  ( 1.0e-7 )
#else
#  ifdef DBL_EPSILON
#    define VDIVIDE_TOL         ( DBL_EPSILON )
#  else
#    define VDIVIDE_TOL         ( 1.0e-20 )
#  endif
#  ifdef FLT_EPSILON
#    define VUNITIZE_TOL        ( FLT_EPSILON )
#  else
#    define VUNITIZE_TOL        ( 1.0e-15 )
#  endif
#endif
#ifdef SHORT_VECTORS
#define VUNITIZE(a) { \
        register double _f = MAGSQ(a); \
        register int _vunitize; \
        if ( !NEAR_ZERO( _f-1.0, VUNITIZE_TOL ) ) { \
                _f = sqrt( _f ); \
                if( _f < VDIVIDE_TOL ) { VSETALL( (a), 0.0 ); } else { \
                        _f = 1.0/_f; \
                        for(_vunitize = 0; _vunitize < 3; _vunitize++) \
                                (a)[_vunitize] *= _f; \
                } \
        } \
}
#else
#define VUNITIZE(a)     { \
        register double _f = MAGSQ(a); \
        if ( !NEAR_ZERO( _f-1.0, VUNITIZE_TOL ) ) { \
                _f = sqrt( _f ); \
                if( _f < VDIVIDE_TOL ) { VSETALL( (a), 0.0 ); } else { \
                        _f = 1.0/_f; \
                        (a)[X] *= _f; (a)[Y] *= _f; (a)[Z] *= _f; \
                } \
        } \
}
#endif

#ifdef SHORT_VECTORS
#define VADD2(a,b,c) VADD2N(a,b,c, 3)
#else
#define VADD2(a,b,c)    { \
                        (a)[X] = (b)[X] + (c)[X];\
                        (a)[Y] = (b)[Y] + (c)[Y];\
                        (a)[Z] = (b)[Z] + (c)[Z]; }
#endif /* SHORT_VECTORS */
#ifdef SHORT_VECTORS
#define VSUB2(a,b,c)    VSUB2N(a,b,c, 3)
#else
#define VSUB2(a,b,c)    { \
                        (a)[X] = (b)[X] - (c)[X];\
                        (a)[Y] = (b)[Y] - (c)[Y];\
                        (a)[Z] = (b)[Z] - (c)[Z]; }
#endif /* SHORT_VECTORS */
/** @brief Subtract `n' length vector at `c' from vector at `b', store result at `a' */
#define VSUB2N(a,b,c,n) \
        { register int _vsub2; \
        for(_vsub2 = 0; _vsub2 < (n); _vsub2++) \
                (a)[_vsub2] = (b)[_vsub2] - (c)[_vsub2]; \
        }

#define V2SUB2(a,b,c)   { \
                        (a)[X] = (b)[X] - (c)[X];\
                        (a)[Y] = (b)[Y] - (c)[Y];}

/* replicated code from color.c to avoid dependency */
#define V3ARGS(a) (a)[X], (a)[Y], (a)[Z]
#define VSET(a, b, c, d) { (a)[X] = (b); (a)[Y] = (c); (a)[Z] = (d); }

/* replicated code from vert_tree.h, vert_tree.c to avoid dependency */
#define VERT_TREE_MAGIC                 0x56455254 /**< VERT from magic.h */
#define TREE_TYPE_VERTS 1
#define TREE_TYPE_VERTS_AND_NORMS 2
#define VERT_BLOCK 512                  /**< @brief number of vertices to malloc per call when building the array */ 
#define BN_CK_VERT_TREE(_p) BU_CKMAG(_p, VERT_TREE_MAGIC, "vert_tree")

static FILE *out_test;
static FILE *out_data;

/* replicated version of strlcpy from FreeBSD */
size_t
strlcpy(char * __restrict dst, const char * __restrict src, size_t dsize)
{
	const char *osrc = src;
	size_t nleft = dsize;

	/* Copy as many bytes as will fit. */
	if (nleft != 0) {
		while (--nleft != 0) {
			if ((*dst++ = *src++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src. */
	if (nleft == 0) {
		if (dsize != 0)
			*dst = '\0';		/* NUL-terminate dst */
		while (*src++)
			;
	}

	return(src - osrc - 1);	/* count does not include NUL */
}

struct vert_root {
     uint32_t magic;
     int tree_type;              /**< @brief vertices or vertices with normals */
     union vert_tree *the_tree;  /**< @brief the actual vertex tree */
     double *the_array;         /**< @brief the array of vertices */
     size_t curr_vert;           /**< @brief the number of vertices currently in the array */
     size_t max_vert;            /**< @brief the current maximum capacity of the array */
};

struct vert_root *create_vert_tree(void){
	struct vert_root *tree;
	tree = (vert_root * )malloc(sizeof(*tree));
	tree->magic = VERT_TREE_MAGIC;
	tree->tree_type = TREE_TYPE_VERTS;
	tree->the_tree = (union vert_tree *)NULL;
	tree->curr_vert = 0;
	tree->max_vert = VERT_BLOCK;
	tree->the_array = (double *)malloc( tree->max_vert * 3 * sizeof( double ));

	return tree;
}

union vert_tree {
    char type;          /* type - leaf or node */
    struct vert_leaf {
        char type;
        int index;      /* index into the array */
    } vleaf;
    struct vert_node {
        char type;
        double cut_val; /* cutting value */
        int coord;      /* cutting coordinate */
        union vert_tree *higher, *lower;        /* subtrees */
    } vnode;
};

/* types for the above "vert_tree" */
#define VERT_LEAF       'l'
#define VERT_NODE       'n'

int
Add_vert( double x, double y, double z, struct vert_root *vert_root, double local_tol_sq )
{
    union vert_tree *ptr, *prev=NULL, *new_leaf, *new_node;
    double diff[4] = {0, 0, 0, 0};
    double vertex[4];

    if ( vert_root->tree_type != TREE_TYPE_VERTS ) {
        fprintf( out_test,  "Error: Add_vert() called for a tree containing vertices and normals\n" );//bu_bomb
    }

    VSET( vertex, x, y, z );

    /* look for this vertex already in the list */
    ptr = vert_root->the_tree;
    while ( ptr ) {
        if ( ptr->type == VERT_NODE ) {
            prev = ptr;
            if ( vertex[ptr->vnode.coord] >= ptr->vnode.cut_val ) {
                ptr = ptr->vnode.higher;
            } else {
                ptr = ptr->vnode.lower;
            }
        } else {
            int ij;

            ij = ptr->vleaf.index*3;
            diff[0] = fabs( vertex[0] - vert_root->the_array[ij] );
            diff[1] = fabs( vertex[1] - vert_root->the_array[ij+1] );
            diff[2] = fabs( vertex[2] - vert_root->the_array[ij+2] );
            if ( (diff[0]*diff[0] + diff[1]*diff[1] + diff[2]*diff[2]) <= local_tol_sq ) {
                /* close enough, use this vertex again */
                return ptr->vleaf.index;
            }
            break;
        }
    }

    /* add this vertex to the list */
    if ( vert_root->curr_vert >= vert_root->max_vert ) {
        /* allocate more memory for vertices */
        vert_root->max_vert += VERT_BLOCK;

        vert_root->the_array = (double *)realloc( vert_root->the_array, sizeof( double ) * vert_root->max_vert * 3);
    }

    VMOVE( &vert_root->the_array[vert_root->curr_vert*3], vertex );

    /* add to the tree also */
	new_leaf = (vert_tree*)malloc(sizeof(*new_leaf));
    new_leaf->vleaf.type = VERT_LEAF;
    new_leaf->vleaf.index = vert_root->curr_vert++;
    if ( !vert_root->the_tree ) {
        /* first vertex, it becomes the root */
        vert_root->the_tree = new_leaf;
    } else if ( ptr && ptr->type == VERT_LEAF ) {
        /* search above ended at a leaf, need to add a node above this leaf and the new leaf */
		new_node = (vert_tree*)malloc(sizeof(*new_node));
        new_node->vnode.type = VERT_NODE;

        /* select the cutting coord based on the biggest difference */
        if ( diff[0] >= diff[1] && diff[0] >= diff[2] ) {
            new_node->vnode.coord = 0;
        } else if ( diff[1] >= diff[2] && diff[1] >= diff[0] ) {
            new_node->vnode.coord = 1;
        } else if ( diff[2] >= diff[1] && diff[2] >= diff[0] ) {
            new_node->vnode.coord = 2;
        }

        /* set the cut value to the mid value between the two vertices */
        new_node->vnode.cut_val = (vertex[new_node->vnode.coord] +
                                   vert_root->the_array[ptr->vleaf.index * 3 + new_node->vnode.coord]) * 0.5;

        /* set the node "lower" and "higher" pointers */
        if ( vertex[new_node->vnode.coord] >=
             vert_root->the_array[ptr->vleaf.index * 3 + new_node->vnode.coord] ) {
            new_node->vnode.higher = new_leaf;
            new_node->vnode.lower = ptr;
        } else {
            new_node->vnode.higher = ptr;
            new_node->vnode.lower = new_leaf;
        }

        if ( ptr == vert_root->the_tree ) {
            /* if the above search ended at the root, redefine the root */
            vert_root->the_tree =  new_node;
        } else {
            /* set the previous node to point to our new one */
            if ( prev->vnode.higher == ptr ) {
                prev->vnode.higher = new_node;
            } else {
                prev->vnode.lower = new_node;
            }
        }
    } else if ( ptr && ptr->type == VERT_NODE ) {
        /* above search ended at a node, just add the new leaf */
        prev = ptr;
        if ( vertex[prev->vnode.coord] >= prev->vnode.cut_val ) {
            if ( prev->vnode.higher ) {
                fprintf( out_test, "higher vertex node already exists in Add_vert()?\n");//bu_bomb
            }
            prev->vnode.higher = new_leaf;
        } else {
            if ( prev->vnode.lower ) {
                fprintf( out_test, "lower vertex node already exists in Add_vert()?\n");//bu_bomb
            }
            prev->vnode.lower = new_leaf;
        }
    } else {
        fprintf( out_test, "*********ERROR********\n" );
    }

    /* return the index into the vertex array */
    return new_leaf->vleaf.index;
}

#define DEG2RAD M_PI/180.0
#define RAD2DEG 180.0/M_PI 	

static int overstrikemode = 0;
static int underscoremode = 0;

static std::vector<double> pt_x;
static std::vector<double> pt_y;
static std::vector<double> pt_z;

static dxf_data dd;

struct state_data {
	int curr_block_indx;
    off_t file_offset;
    int state;
    int sub_state;
    double xform[16];
};

static std::vector<state_data> state_stack;
static struct state_data *curr_state;
static int curr_color=7;
static int ignore_colors = 0;
static char *curr_layer_name;
static int color_by_layer = 0;		/* flag, if set, colors are set by layer */

struct layer {
    char *name;			/* layer name */
    int color_number;		/* color */
    struct vert_root *vert_tree; /* root of vertex tree */
    int *part_tris;			/* list of triangles for current part */
    size_t max_tri;			/* number of triangles currently malloced */
    size_t curr_tri;			/* number of triangles currently being used */
    size_t line_count;
    size_t solid_count;
    size_t polyline_count;
    size_t lwpolyline_count;
    size_t ellipse_count;
    size_t circle_count;
    size_t spline_count;
    size_t arc_count;
    size_t text_count;
    size_t mtext_count;
    size_t attrib_count;
    size_t dimension_count;
    size_t leader_count;
    size_t face3d_count;
    size_t point_count;
    struct std::list<uint32_t> solids;
    struct model *m;
    struct shell *s;
};


struct block {
    std::string block_name;
    off_t offset;
    char handle[17];
    double base[3];
	
	block(const block& blk){
		strlcpy(handle, blk.handle, sizeof(handle));
		block_name = blk.block_name;
		VMOVE(base, blk.base);
		offset = blk.offset;
	}
	block(): block_name(std::string()), offset(off_t()){
		for(int i = 0; i < 3; i ++){
			base[i] = -1;
		}
		for(int i = 0; i < 17; i++){
			handle[i] = 0;
		}
	}
	block(off_t file_offset): block_name(std::string()){
		offset = file_offset;
		for(int i = 0; i < 3; i ++){
			base[i] = -1;
		}
		for(int i = 0; i < 17; i++){
			handle[i] = 0;
		}
	}

	bool empty(){
		if(this->block_name.empty()){
			if(this->offset == off_t()){
				for(int i = 0; i < 3; i ++){
					if(base[i] != -1){
						return false;
					}
				}
				for(int i = 0; i < 17; i++){
					if(handle[i] != 0){
						return false;
					}
				}
				return true;
			}
			return false;
		}
		return false;
	}
};

static std::vector<block> block_list;
static block *curr_block;
static int indx;

//static struct layer **layers=NULL;
static std::vector<layer> layers;
static int max_layers;
static int next_layer;
static int curr_layer;

/* SECTIONS (states) */
#define UNKNOWN_SECTION		0
#define HEADER_SECTION		1
#define CLASSES_SECTION		2
#define TABLES_SECTION		3
#define BLOCKS_SECTION		4
#define ENTITIES_SECTION	5
#define OBJECTS_SECTION		6
#define THUMBNAILIMAGE_SECTION	7
#define NUM_SECTIONS		8

/* states for the ENTITIES section */
#define UNKNOWN_ENTITY_STATE		0
#define POLYLINE_ENTITY_STATE		1
#define POLYLINE_VERTEX_ENTITY_STATE	2
#define FACE3D_ENTITY_STATE		3
#define LINE_ENTITY_STATE		4
#define INSERT_ENTITY_STATE		5
#define POINT_ENTITY_STATE		6
#define CIRCLE_ENTITY_STATE		7
#define ARC_ENTITY_STATE		8
#define DIMENSION_ENTITY_STATE		9
#define TEXT_ENTITY_STATE		10
#define SOLID_ENTITY_STATE		11
#define LWPOLYLINE_ENTITY_STATE		12
#define MTEXT_ENTITY_STATE		13
#define LEADER_ENTITY_STATE		14
#define ATTRIB_ENTITY_STATE		15
#define ATTDEF_ENTITY_STATE		16
#define ELLIPSE_ENTITY_STATE		17
#define SPLINE_ENTITY_STATE		18
#define NUM_ENTITY_STATES		19

/* POLYLINE flags */
static int polyline_flag = 0;
#define POLY_CLOSED		1
#define POLY_CURVE_FIT		2
#define POLY_SPLINE_FIT		4
#define POLY_3D			8
#define POLY_3D_MESH		16
#define POLY_CLOSED_MESH	32
#define POLY_FACE_MESH		64
#define POLY_PATTERN		128

/* POLYLINE VERTEX flags */
#define POLY_VERTEX_EXTRA	1
#define POLY_VERTEX_CURVE	2
#define POLY_VERTEX_SPLINE_V	8
#define POLY_VERTEX_SPLINE_C	16
#define POLY_VERTEX_3D_V	32
#define POLY_VERTEX_3D_M	64
#define POLY_VERTEX_FACE	128

/* SPLINE flags */
#define SPLINE_CLOSED		1
#define SPLINE_PERIODIC		2
#define SPLINE_RATIONAL		4
#define SPLINE_PLANAR		8
#define SPLINE_LINEAR		16

/* states for the TABLES section */
#define UNKNOWN_TABLE_STATE	0
#define LAYER_TABLE_STATE	1
#define NUM_TABLE_STATES	2

static double *polyline_verts=NULL;
static int polyline_vertex_count = 0;
static int polyline_vertex_max = 0;
static int mesh_m_count = 0;
static int mesh_n_count = 0;
static int *polyline_vert_indices=NULL;
static int polyline_vert_indices_count = 0;
static int polyline_vert_indices_max = 0;
#define PVINDEX(_i, _j)	((_i)*mesh_n_count + (_j))
#define POLYLINE_VERTEX_BLOCK	10

static double pts[4][3];

#define UNKNOWN_ENTITY 0
#define POLYLINE_VERTEX 1

static int invisible = 0;

#define ERROR_FLAG	-999
#define EOF_FLAG	-998

#define TOL_SQ 0.00001
#define MAX_LINE_SIZE 2050
char line[MAX_LINE_SIZE];

const char *usage = "Usage: dxf-g [-c] [-d] [-v] [-t tolerance] [-s scale_factor] input_file.dxf output_file.g\n";

static FILE *dxf;
//static struct rt_wdb *out_fp;
static FILE *out_fp;
static char *output_file;
static char *dxf_file;
static int verbose = 0;
static double tol = 0.01;
static double tol_sq;
static char *base_name;
static char tmp_name[256];
static int segs_per_circle=32;
static int splineSegs=16;
static double sin_delta, cos_delta;
static double delta_angle;
static double **circle_pts;
static double scale_factor;
static std::list<uint32_t> free_hd;

#define TRI_BLOCK 512			/* number of triangles to malloc per call */

static int (*process_code[NUM_SECTIONS])(int code);
static int (*process_entities_code[NUM_ENTITY_STATES])(int code);
static int (*process_tables_sub_code[NUM_TABLE_STATES])(int code);

static int *int_ptr=NULL;
static int units = 0;
static double units_conv[]={
    /* 0 */	1.0,
    /* 1 */	25.4,
    /* 2 */	304.8,
    /* 3 */	1609344.0,
    /* 4 */	1.0,
    /* 5 */	10.0,
    /* 6 */	1000.0,
    /* 7 */	1000000.0,
    /* 8 */	0.0000254,
    /* 9 */	0.0254,
    /* 10 */ 914.4,
    /* 11 */ 1.0e-7,
    /* 12 */ 1.0e-6,
    /* 13 */ 1.0e-3,
    /* 14 */ 100.0,
    /* 15 */ 10000.0,
    /* 16 */ 100000.0,
    /* 17 */ 1.0e+12,
    /* 18 */ 1.495979e+14,
    /* 19 */ 9.460730e+18,
    /* 20 */ 3.085678e+19
};

/* replicated code from mat.h to avoid dependency */
void
bn_mat_angles(
    register double *mat,
    double alpha_in,
    double beta_in,
    double ggamma_in)
{
    double alpha, beta, ggamma;
    double calpha, cbeta, cgamma;
    double salpha, sbeta, sgamma;

    if (NEAR_ZERO(alpha_in, 0.0) && NEAR_ZERO(beta_in, 0.0) && NEAR_ZERO(ggamma_in, 0.0)) {
        MAT_IDN(mat);
        return;
    }

    alpha = alpha_in * DEG2RAD;
    beta = beta_in * DEG2RAD;
    ggamma = ggamma_in * DEG2RAD;

    calpha = cos(alpha);
    cbeta = cos(beta);
    cgamma = cos(ggamma);

    /* sine of "180*DEG2RAD" will not be exactly zero and will
     * result in errors when some codes try to convert this back to
     * azimuth and elevation.  do_frame() uses this technique!!!
     */
    if (ZERO(alpha_in - 180.0)) {
        salpha = 0.0;
    } else {
        salpha = sin(alpha);
    }

    if (ZERO(beta_in - 180.0)) {
        sbeta = 0.0;
    } else {
        sbeta = sin(beta);
    }

    if (ZERO(ggamma_in - 180.0)) {
        sgamma = 0.0;
    } else {
        sgamma = sin(ggamma);
    }

    mat[0] = cbeta * cgamma;
    mat[1] = -cbeta * sgamma;
    mat[2] = sbeta;
    mat[3] = 0.0;

    mat[4] = salpha * sbeta * cgamma + calpha * sgamma;
    mat[5] = -salpha * sbeta * sgamma + calpha * cgamma;
    mat[6] = -salpha * cbeta;
    mat[7] = 0.0;

    mat[8] = salpha * sgamma - calpha * sbeta * cgamma;
    mat[9] = salpha * cgamma + calpha * sbeta * sgamma;
    mat[10] = calpha * cbeta;
    mat[11] = 0.0;
    mat[12] = mat[13] = mat[14] = 0.0;
    mat[15] = 1.0;
}
void
bn_mat_mul(register double o[16], register const double a[16], register const double b[16])
{
    o[ 0] = a[ 0] * b[ 0] + a[ 1] * b[ 4] + a[ 2] * b[ 8] + a[ 3] * b[12];
    o[ 1] = a[ 0] * b[ 1] + a[ 1] * b[ 5] + a[ 2] * b[ 9] + a[ 3] * b[13];
    o[ 2] = a[ 0] * b[ 2] + a[ 1] * b[ 6] + a[ 2] * b[10] + a[ 3] * b[14];
    o[ 3] = a[ 0] * b[ 3] + a[ 1] * b[ 7] + a[ 2] * b[11] + a[ 3] * b[15];

    o[ 4] = a[ 4] * b[ 0] + a[ 5] * b[ 4] + a[ 6] * b[ 8] + a[ 7] * b[12];
    o[ 5] = a[ 4] * b[ 1] + a[ 5] * b[ 5] + a[ 6] * b[ 9] + a[ 7] * b[13];
    o[ 6] = a[ 4] * b[ 2] + a[ 5] * b[ 6] + a[ 6] * b[10] + a[ 7] * b[14];
    o[ 7] = a[ 4] * b[ 3] + a[ 5] * b[ 7] + a[ 6] * b[11] + a[ 7] * b[15];

    o[ 8] = a[ 8] * b[ 0] + a[ 9] * b[ 4] + a[10] * b[ 8] + a[11] * b[12];
    o[ 9] = a[ 8] * b[ 1] + a[ 9] * b[ 5] + a[10] * b[ 9] + a[11] * b[13];
    o[10] = a[ 8] * b[ 2] + a[ 9] * b[ 6] + a[10] * b[10] + a[11] * b[14];
    o[11] = a[ 8] * b[ 3] + a[ 9] * b[ 7] + a[10] * b[11] + a[11] * b[15];

    o[12] = a[12] * b[ 0] + a[13] * b[ 4] + a[14] * b[ 8] + a[15] * b[12];
    o[13] = a[12] * b[ 1] + a[13] * b[ 5] + a[14] * b[ 9] + a[15] * b[13];
    o[14] = a[12] * b[ 2] + a[13] * b[ 6] + a[14] * b[10] + a[15] * b[14];
    o[15] = a[12] * b[ 3] + a[13] * b[ 7] + a[14] * b[11] + a[15] * b[15];
}

/* Added functions for list operation */
bool block_list_is_head(block bl, std::vector<block> list){
	if(!list.empty()){
		if(bl.block_name != list.front().block_name)
			return false;
		else if(!strcmp(bl.handle, list.front().handle))
			return false;
		else if(bl.offset != list.front().offset)
			return false;
		else{
			for(int i = 0; i < 3; i++){
				if(bl.base[i] != list.front().base[i])
					return false;
			}
			return true;
		}
	}
	return true;

}

/* function for undefined NEAR_EQUAL */
bool NEAR_EQUAL(double a, double b, double local_tol_sql){
	double diff = fabs(a - b);
	if(diff <= local_tol_sql){
		return true;
	}
	return false;
}
/* function for C style string to C++ string to avoid initialization */
std::string Char2String(const char* line){
	std::string str(line);
	return str;
}

static char *
make_brlcad_name(const char *nameline)
{
    char *name;
    char *c;

    name = strdup(nameline);

    c = name;
    while (*c != '\0') {
		if (*c == '/' || *c == '[' || *c == ']' || *c == '*' || isspace((int)*c)) {
			*c = '_';
		}
		c++;
    }

    return name;
}


static void
get_layer()
{
    int i;
    int old_layer=curr_layer;

    if (verbose) {
	fprintf(out_test, "get_layer(): state = %d, substate = %d\n", curr_state->state, curr_state->sub_state);
    }

    /* do we already have a layer by this name and color */
    curr_layer = -1;
    for (i = 1; i < next_layer; i++) {
		if (!color_by_layer && !ignore_colors && curr_color != 256) {
			if (layers[i].color_number == curr_color && layers[i].name == curr_layer_name) {
				curr_layer = i;
				break;
			}
		} else {
			if (layers[i].name == curr_layer_name) {
				curr_layer = i;
				break;
			}
		}
    }

    if (curr_layer == -1) {
	/* add a new layer */
	if (next_layer >= max_layers) {
	    if (verbose) {
		fprintf(out_test, "Creating new block of layers\n");
	    }
	    max_layers += 5;
	    //layers = (struct layer **)realloc(layers, max_layers*sizeof(struct layer *));
	    for (i = 0; i < 5; i++) {
			layers.push_back(layer());
			//layers[max_layers-i-1] = (layer *)malloc(sizeof(*layers[max_layers-i-1])); //bu_alloc
	    }
	}
	curr_layer = next_layer++;
	if (verbose) {
	    fprintf(out_test, "New layer: %s, color number: %d", line, curr_color);
	}
	layers[curr_layer].name = strdup(curr_layer_name);
	layers[curr_layer].name = curr_layer_name;
	if (curr_state->state == ENTITIES_SECTION &&
	    (curr_state->sub_state == POLYLINE_ENTITY_STATE ||
	     curr_state->sub_state == POLYLINE_VERTEX_ENTITY_STATE)) {
	    layers[curr_layer].vert_tree = layers[old_layer].vert_tree;
	} else {
	    layers[curr_layer].vert_tree = create_vert_tree(); //bn_vert_tree_create();

	}
	layers[curr_layer].color_number = curr_color;
	//bu_ptbl_init(&layers[curr_layer].solids, 8, "layers[curr_layer].solids");
	if (verbose) {
	    fprintf(out_test, "\tNew layer name: %s\n", layers[curr_layer].name);
	}
    }

    if (verbose && curr_layer != old_layer) {
	fprintf(out_test, "changed to layer #%d, (m = %p, s=%p)\n",
	       curr_layer,
	       (void *)layers[curr_layer].m,
	       (void *)layers[curr_layer].s);
    }
}


/* routine to add a new triangle to the current part */
void
add_triangle(int v1, int v2, int v3, int layer)
{
    if (verbose) {
	fprintf(out_test, "Adding triangle %d %d %d, to layer %s\n", v1, v2, v3, layers[layer].name);
    }
    if (v1 == v2 || v2 == v3 || v3 == v1) {
	if (verbose) {
	    fprintf(out_test, "\tSkipping degenerate triangle\n");
	}
	return;
    }
    if (layers[layer].curr_tri >= layers[layer].max_tri) {
	/* allocate more memory for triangles */
	layers[layer].max_tri += TRI_BLOCK;
	layers[layer].part_tris = (int *)realloc(layers[layer].part_tris, sizeof(int) * layers[layer].max_tri * 3);
    }

    /* fill in triangle info */
    layers[layer].part_tris[layers[layer].curr_tri*3 + 0] = v1;
    layers[layer].part_tris[layers[layer].curr_tri*3 + 1] = v2;
    layers[layer].part_tris[layers[layer].curr_tri*3 + 2] = v3;

    /* increment count */
    layers[layer].curr_tri++;
}


static int
process_unknown_code(int code)
{
    switch (code) {
	case 999:	/* comment */
	    printf("%s\n", line);
	    break;
	case 0:		/* text string */
	    if (!strncmp(line, "SECTION", 7)) {
		curr_state->state = UNKNOWN_SECTION;
		break;
	    } else if (!strncmp(line, "ENDSEC", 6)) {
		curr_state->state = UNKNOWN_SECTION;
		break;
	    }
	    break;
	case 2:		/* name */
	    if (!strncmp(line, "HEADER", 6)) {
		curr_state->state = HEADER_SECTION;
		if (verbose) {
		    fprintf(out_test, "Change state to %d\n", curr_state->state);
		}
		break;
	    } else if (!strncmp(line, "CLASSES", 7)) {
		curr_state->state = CLASSES_SECTION;
		if (verbose) {
		    fprintf(out_test, "Change state to %d\n", curr_state->state);
		}
		break;
	    } else if (!strncmp(line, "TABLES", 6)) {
		curr_state->state = TABLES_SECTION;
		curr_state->sub_state = UNKNOWN_TABLE_STATE;
		if (verbose) {
		    fprintf(out_test, "Change state to %d\n", curr_state->state);
		}
		break;
	    } else if (!strncmp(line, "BLOCKS", 6)) {
		curr_state->state = BLOCKS_SECTION;
		if (verbose) {
		    fprintf(out_test, "Change state to %d\n", curr_state->state);
		}
		break;
	    } else if (!strncmp(line, "ENTITIES", 8)) {
		curr_state->state = ENTITIES_SECTION;
		curr_state->sub_state =UNKNOWN_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "Change state to %d\n", curr_state->state);
		}
		break;
	    } else if (!strncmp(line, "OBJECTS", 7)) {
		curr_state->state = OBJECTS_SECTION;
		if (verbose) {
		    fprintf(out_test, "Change state to %d\n", curr_state->state);
		}
		break;
	    } else if (!strncmp(line, "THUMBNAILIMAGE", 14)) {
		curr_state->state = THUMBNAILIMAGE_SECTION;
		if (verbose) {
		    fprintf(out_test, "Change state to %d\n", curr_state->state);
		}
		break;
	    }
	    break;
	case 62:	/* color number */
	    curr_color = atoi(line);
	    break;
    }
    return 0;
}


static int
process_header_code(int code)
{
    switch (code) {
	case 999:	/* comment */
	    printf("%s\n", line);
	    break;
	case 0:		/* text string */
	    if (!strncmp(line, "SECTION", 7)) {
		curr_state->state = UNKNOWN_SECTION;
		break;
	    } else if (!strncmp(line, "ENDSEC", 6)) {
		curr_state->state = UNKNOWN_SECTION;
		break;
	    }
	    break;
	case 9:		/* variable name */
	    if (!strncmp(line, "$INSUNITS", 9)) {
		int_ptr = &units;
	    } else if (!strcmp(line, "$CECOLOR")) {
		int_ptr = &color_by_layer;
	    } else if (!strcmp(line, "$SPLINESEGS")) {
		int_ptr = &splineSegs;
	    }
	    break;
	case 70:
	case 62:
	    if (int_ptr) {
		(*int_ptr) = atoi(line);
	    }
	    int_ptr = NULL;
	    break;
    }

    return 0;
}


static int
process_classes_code(int code)
{
    switch (code) {
	case 999:	/* comment */
	    printf("%s\n", line);
	    break;
	case 0:		/* text string */
	    if (!strncmp(line, "SECTION", 7)) {
		curr_state->state = UNKNOWN_SECTION;
		break;
	    } else if (!strncmp(line, "ENDSEC", 6)) {
		curr_state->state = UNKNOWN_SECTION;
		break;
	    }
	    break;
    }

    return 0;
}


static int
process_tables_unknown_code(int code)
{
    switch (code) {
	case 999:	/* comment */
	    printf("%s\n", line);
	    break;
	case 0:		/* text string */
	    if (!strncmp(line, "LAYER",5)) {
		if (curr_layer_name) {
		    free(curr_layer_name);
		    curr_layer_name = NULL;
		}
		curr_color = 0;
		curr_state->sub_state = LAYER_TABLE_STATE;
		break;
	    } else if (!strncmp(line, "ENDTAB", 6)) {
		if (curr_layer_name) {
		    free(curr_layer_name);
		    curr_layer_name = NULL;
		}
		curr_color = 0;
		curr_state->sub_state = UNKNOWN_TABLE_STATE;
		break;
	    } else if (!strncmp(line, "SECTION", 7)) {
		curr_state->state = UNKNOWN_SECTION;
		break;
	    } else if (!strncmp(line, "ENDSEC", 6)) {
		curr_state->state = UNKNOWN_SECTION;
		break;
	    }
	    break;
    }

    return 0;
}


static int
process_tables_layer_code(int code)
{
    switch (code) {
	case 999:	/* comment */
	    printf("%s\n", line);
	    break;
	case 2:		/* layer name */
	    if (curr_layer_name) {
		free(curr_layer_name);
	    }
	    curr_layer_name = make_brlcad_name(line);
	    if (verbose) {
		fprintf(out_test, "In LAYER in TABLES, layer name = %s\n", curr_layer_name);
	    }
	    break;
	case 62:	/* layer color */
	    curr_color = atoi(line);
	    if (verbose) {
		fprintf(out_test, "In LAYER in TABLES, layer color = %d\n", curr_color);
	    }
	    break;
	case 0:		/* text string */
	    if (curr_layer_name && curr_color) {
		get_layer();
	    }

		table_struct ts; 
		ts.color = curr_color;
		ts.layer_name = std::string(curr_layer_name);
		dd.table_vector.emplace_back(ts);

	    if (curr_layer_name) {
		free(curr_layer_name);
		curr_layer_name = NULL;
	    }
	    curr_color = 0;
	    curr_state->sub_state = UNKNOWN_TABLE_STATE;
	    return process_tables_unknown_code(code);
    }

    return 0;
}


static int
process_tables_code(int code)
{
    return process_tables_sub_code[curr_state->sub_state](code);
}


static int
process_blocks_code(int code)
{
    size_t len;
    int coord;

    switch (code) {
	case 999:	/* comment */
	    printf("%s\n", line);
	    break;
	case 0:		/* text string */
	    if (!strncmp(line, "SECTION", 7)) {
		curr_state->state = UNKNOWN_SECTION;
		break;
	    } else if (!strncmp(line, "ENDSEC", 6)) {
		curr_state->state = UNKNOWN_SECTION;
		break;
	    } else if (!strcmp(line, "ENDBLK")) {
		indx = -1;
		delete curr_block;
		curr_block = nullptr;
		break;
	    } else if (!strncmp(line, "BLOCK", 5)) {
		/* start of a new block */

		block tmp(ftell(dxf));
		block_list.emplace_back(tmp);
		indx = block_list.size()-1;
		break;
	    }
	    break;
	case 2:		/* block name */
	    if (indx != -1) {
			if(block_list.at(indx).block_name.empty())
				block_list.at(indx).block_name = std::string(strdup(line));
			if (verbose) {
				fprintf(out_test, "BLOCK %s begins at %jd\n",
				block_list.at(indx).block_name.c_str(),
				(intmax_t)block_list.at(indx).offset);
		}
	    }
	    break;
	case 5:		/* block handle */
	    if (indx != -1) 
			if(!strcmp("" ,block_list.at(indx).handle)) {
				len = strlen(line);
				V_MIN(len, 16);
				strlcpy(block_list.at(indx).handle, line, len);
			}
	    break;
	case 10:
	case 20:
	case 30:
	    if (indx != -1) {
		coord = code / 10 - 1;
		block_list.at(indx).base[coord] = atof(line) * units_conv[units] * scale_factor;
	    }
	    break;
    }

    return 0;
}


void
add_polyline_vertex(double x, double y, double z)
{
    if (!polyline_verts) {
	polyline_verts = (double *)malloc(POLYLINE_VERTEX_BLOCK*3*sizeof(double));//, "polyline_verts");
	polyline_vertex_count = 0;
	polyline_vertex_max = POLYLINE_VERTEX_BLOCK;
    } else if (polyline_vertex_count >= polyline_vertex_max) {
	polyline_vertex_max += POLYLINE_VERTEX_BLOCK;
	polyline_verts = (double *)realloc(polyline_verts, polyline_vertex_max * 3 * sizeof(double)); //, "polyline_verts");
    }

    VSET(&polyline_verts[polyline_vertex_count*3], x, y, z);
    polyline_vertex_count++;

    if (verbose) {
	fprintf(out_test, "Added polyline vertex (%g %g %g) #%d\n", x, y, z, polyline_vertex_count);
    }
}


static int
process_point_entities_code(int code)
{
    static double pt[3];
    double tmp_pt[3];
    int coord;

    switch (code) {
	case 8:		/* layer name */
	    if (curr_layer_name) {
		free(curr_layer_name);
	    }
	    curr_layer_name = make_brlcad_name(line);
	    break;
	case 10:
	case 20:
	case 30:
	    coord = code / 10 - 1;
	    pt[coord] = atof(line) * units_conv[units] * scale_factor;
	    break;
	case 62:	/* color number */
	    curr_color = atoi(line);
	    break;
	case 0:
	    get_layer();

		point_struct pt_struct; 
		VMOVE(pt_struct.pt, pt);
		pt_struct.layer_name = std::string(curr_layer_name);
		dd.point_vector.emplace_back(pt_struct);

	     layers[curr_layer].point_count++;
	    MAT4X3PNT(tmp_pt, curr_state->xform, pt);
	    sprintf(tmp_name, "point.%lu", (long unsigned int)layers[curr_layer].point_count);
	    curr_state->sub_state = UNKNOWN_ENTITY_STATE;
	    process_entities_code[curr_state->sub_state](code);
	    break;
    }

    return 0;
}


static int
process_entities_polyline_vertex_code(int code)
{
    static double x, y, z;
    static int face[4];
    static int vertex_flag;
    int coord;

    switch (code) {
	case -1:	/* initialize */
	    face[0] = 0;
	    face[1] = 0;
	    face[2] = 0;
	    face[3] = 0;
	    vertex_flag = 0;
	    return 0;
	case 8:		/* layer name */
	    if (curr_layer_name) {
		free(curr_layer_name);
	    }
	    curr_layer_name = make_brlcad_name(line);
	    break;
	case 70:	/* vertex flag */
	    vertex_flag = atoi(line);
	    break;
	case 71:
	case 72:
	case 73:
	case 74:
	    coord = (code % 70) - 1;
	    face[coord] = abs(atoi(line));
	    break;
	case 0:
	{
		get_layer();

		polyline_vertex_struct pvs;
		pvs.x = x;
		pvs.y = y;
		pvs.z = z;
		VMOVE(pvs.face, face);
		pvs.face[3] = face[3];
		pvs.color = curr_color;
		pvs.layer_name = std::string(curr_layer_name);
		dd.polyline_vertex_vector.emplace_back(pvs);

	    if (vertex_flag == POLY_VERTEX_FACE) {
		add_triangle(polyline_vert_indices[face[0]-1],
			     polyline_vert_indices[face[1]-1],
			     polyline_vert_indices[face[2]-1],
			     curr_layer);
		if (face[3] > 0) {
		    add_triangle(polyline_vert_indices[face[2]-1],
				 polyline_vert_indices[face[3]-1],
				 polyline_vert_indices[face[0]-1],
				 curr_layer);
		}
	    } else if (vertex_flag & POLY_VERTEX_3D_M) {
		double tmp_pt1[3], tmp_pt2[3];
		if (polyline_vert_indices_count >= polyline_vert_indices_max) {
		    polyline_vert_indices_max += POLYLINE_VERTEX_BLOCK;
		    polyline_vert_indices = (int *)realloc(polyline_vert_indices,
							        polyline_vert_indices_max * sizeof(int));
		}
		VSET(tmp_pt1, x, y, z);
		MAT4X3PNT(tmp_pt2, curr_state->xform, tmp_pt1);
		polyline_vert_indices[polyline_vert_indices_count++] = Add_vert(tmp_pt2[X], tmp_pt2[Y], tmp_pt2[Z],layers[curr_layer].vert_tree, tol_sq);
		if (verbose) {
		    fprintf(out_test, "Added 3D mesh vertex (%f %f %f) index = %d, number = %d\n",
			   x, y, z, polyline_vert_indices[polyline_vert_indices_count-1],
			   polyline_vert_indices_count-1);
		}
	    } else {
		add_polyline_vertex(x, y, z);
	    }
	    curr_state->sub_state = POLYLINE_ENTITY_STATE;
	    if (verbose) {
		fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
	    }
	    return process_entities_code[curr_state->sub_state](code);
		}
	case 999:	/* comment */
	    printf("%s\n", line);
	    break;
	case 10:
	    x = atof(line) * units_conv[units] * scale_factor;
	    break;
	case 20:
	    y = atof(line) * units_conv[units] * scale_factor;
	    break;
	case 30:
	    z = atof(line) * units_conv[units] * scale_factor;
	    break;
	case 62:	/* color number */
	    curr_color = atoi(line);
	    break;
    }

    return 0;
}


static int
process_entities_polyline_code(int code)
{

    switch (code) {
	case 999:	/* comment */
	    printf("%s\n", line);
	    break;
	case 0:		/* text string */
	{
	    get_layer();

		polyline_struct ps;
		ps.mesh_m_count = mesh_m_count;
		ps.mesh_n_count = mesh_n_count;
		ps.polyline_flag = polyline_flag;
		ps.invisible = invisible;
		ps.color = curr_color;
		ps.layer_name = std::string(curr_layer_name);
		dd.polyline_vector.emplace_back(ps);

	    if (!strncmp(line, "SEQEND", 6)) {
		/* build any polyline meshes here */
		if (polyline_flag & POLY_3D_MESH) {
		    if (polyline_vert_indices_count == 0) {
			return 0;
		    } else if (polyline_vert_indices_count != mesh_m_count * mesh_n_count) {
			fprintf(out_test, "Incorrect number of vertices for polygon mesh!!!\n");
			polyline_vert_indices_count = 0;
		    } else {
			int i, j;

			if (polyline_vert_indices_count >= polyline_vert_indices_max) {
			    polyline_vert_indices_max = ((polyline_vert_indices_count % POLYLINE_VERTEX_BLOCK) + 1) *
				POLYLINE_VERTEX_BLOCK;
			    polyline_vert_indices = (int *)realloc(polyline_vert_indices,
								      polyline_vert_indices_max * sizeof(int));
			}

			if (mesh_m_count < 2) {
			    if (mesh_n_count > 4) {
				fprintf(out_test, "Cannot handle polyline meshes with m<2 and n>4\n");
				polyline_vert_indices_count = 0;
				polyline_vert_indices_count = 0;
				break;
			    }
			    if (mesh_n_count < 3) {
				polyline_vert_indices_count = 0;
				polyline_vert_indices_count = 0;
				break;
			    }
			    add_triangle(polyline_vert_indices[0],
					 polyline_vert_indices[1],
					 polyline_vert_indices[2],
					 curr_layer);
			    if (mesh_n_count == 4) {
				add_triangle(polyline_vert_indices[2],
					     polyline_vert_indices[3],
					     polyline_vert_indices[0],
					     curr_layer);
			    }
			}

			for (j = 1; j < mesh_n_count; j++) {
			    for (i = 1; i < mesh_m_count; i++) {
				add_triangle(polyline_vert_indices[PVINDEX(i-1, j-1)],
					     polyline_vert_indices[PVINDEX(i-1, j)],
					     polyline_vert_indices[PVINDEX(i, j-1)],
					     curr_layer);
				add_triangle(polyline_vert_indices[PVINDEX(i-1, j-1)],
					     polyline_vert_indices[PVINDEX(i, j-1)],
					     polyline_vert_indices[PVINDEX(i, j)],
					     curr_layer);
			    }
			}
			polyline_vert_indices_count = 0;
			polyline_vertex_count = 0;
		    }
		} else {
		    struct edgeuse *eu;
		    struct vertex *v0 = NULL, *v1 = NULL, *v2 = NULL;

		    if (polyline_vertex_count > 1) {
			int i;

			// if (!layers[curr_layer].m) {
			//     create_nmg();
			// }

			// for (i = 0; i < polyline_vertex_count-1; i++) {
			//     eu = nmg_me(v1, v2, layers[curr_layer].s);
			//     if (i == 0) {
			// 	v1 = eu->vu_p->v_p;
			// 	nmg_vertex_gv(v1, polyline_verts);
			// 	v0 = v1;
			//     }
			//     v2 = eu->eumate_p->vu_p->v_p;
			//     nmg_vertex_gv(v2, &polyline_verts[(i+1)*3]);
			//     if (verbose) {
			// 	fprintf(out_test, "Wire edge (polyline): (%g %g %g) <-> (%g %g %g)\n",
			// 	       V3ARGS(v1->vg_p->coord),
			// 	       V3ARGS(v2->vg_p->coord));
			//     }
			//     v1 = v2;
			//     v2 = NULL;
			// }

			// if (polyline_flag & POLY_CLOSED) {
			//     v2 = v0;
			//     (void)nmg_me(v1, v2, layers[curr_layer].s);
			//      if (verbose) {
			// 	 fprintf(out_test, "Wire edge (closing polyline): (%g %g %g) <-> (%g %g %g)\n",
			// 	        V3ARGS(v1->vg_p->coord),
			// 	        V3ARGS(v2->vg_p->coord));
			//      }
			//}
		    }
		    polyline_vert_indices_count = 0;
		    polyline_vertex_count = 0;
		}

		layers[curr_layer].polyline_count++;
		curr_state->state = ENTITIES_SECTION;
		curr_state->sub_state = UNKNOWN_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
		}
		break;
	    } else if (!strncmp(line, "VERTEX", 6)) {
		if (verbose)
				fprintf(out_test, "Found a POLYLINE VERTEX\n");
		curr_state->sub_state = POLYLINE_VERTEX_ENTITY_STATE;
		process_entities_code[POLYLINE_VERTEX_ENTITY_STATE](-1);
		break;
	    } else {
		if (verbose) {
		    fprintf(out_test, "Unrecognized text string while in polyline entity: %s\n", line);
		}
		break;
	    }
	}
	case 70:	/* polyline flag */
	    polyline_flag = atoi(line);
	    break;
	case 71:
	    mesh_m_count = atoi(line);
	    break;
	case 72:
	    mesh_n_count = atoi(line);
	    break;
	case 62:	/* color number */
	    curr_color = atoi(line);
	    break;
	case 60:
	    invisible = atoi(line);
	    break;
	case 8:		/* layer name */
	    if (curr_layer_name) {
		free(curr_layer_name);
	    }
	    curr_layer_name = make_brlcad_name(line);
	    break;
    }

    return 0;
}


static int
process_entities_unknown_code(int code)
{
    invisible = 0;

    switch (code) {
	case 999:	/* comment */
	    // printf("%s\n", line);
	    break;
	case 0:		/* text string */
	    if (!strncmp(line, "SECTION", 7)) {
		curr_state->state = UNKNOWN_SECTION;
		break;
	    } else if (!strncmp(line, "ENDSEC", 6)) {
		curr_state->state = UNKNOWN_SECTION;
		break;
	    } else if (!strncmp(line, "POLYLINE", 8)) {
		if (verbose)
		    fprintf(out_test, "Found a POLYLINE\n");
		curr_state->sub_state = POLYLINE_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
		}
		break;
	    } else if (!strncmp(line, "LWPOLYLINE", 10)) {
		if (verbose)
		    fprintf(out_test, "Found a LWPOLYLINE\n");
		curr_state->sub_state = LWPOLYLINE_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
		}
		break;
	    } else if (!strncmp(line, "3DFACE", 6)) {
		curr_state->sub_state = FACE3D_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
		}
		break;
	    } else if (!strcmp(line, "CIRCLE")) {
		curr_state->sub_state = CIRCLE_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
		}
		break;
	    } else if (!strcmp(line, "ELLIPSE")) {
		curr_state->sub_state = ELLIPSE_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
		}
		break;
	    } else if (!strcmp(line, "SPLINE")) {
		curr_state->sub_state = SPLINE_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
		}
		break;
	    } else if (!strcmp(line, "ARC")) {
		curr_state->sub_state = ARC_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
		}
		break;
	    } else if (!strcmp(line, "DIMENSION")) {
		curr_state->sub_state = DIMENSION_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
		}
		break;
	    } else if (!strncmp(line, "LINE", 4)) {
		curr_state->sub_state = LINE_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
		}
		break;
	    } else if (!strcmp(line, "POINT")) {
		curr_state->sub_state = POINT_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
		}
		break;
	    } else if (!strcmp(line, "LEADER")) {
		curr_state->sub_state = LEADER_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
		}
		break;
	    } else if (!strcmp(line, "MTEXT")) {
		curr_state->sub_state = MTEXT_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
		}
		break;
	    } else if (!strcmp(line, "TEXT")) {
		curr_state->sub_state = TEXT_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
		}
		break;
	    } else if (!strcmp(line, "ATTRIB")) {
		curr_state->sub_state = ATTRIB_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
		}
		break;
	    } else if (!strcmp(line, "ATTDEF")) {
		curr_state->sub_state = ATTDEF_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
		}
		break;
	    } else if (!strcmp(line, "SOLID")) {
		curr_state->sub_state = SOLID_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
		}
		break;
	    } else if (!strncmp(line, "VIEWPORT", 8)) {
		/* not a useful entity, just ignore it */
		break;
	    } else if (!strncmp(line, "INSERT", 6)) {
		curr_state->sub_state = INSERT_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "sub_state changed to %d\n", curr_state->sub_state);
		}
		break;
	    } else if (!strcmp(line, "ENDBLK")) {
		/* found end of an inserted block, pop the state stack */
		curr_state = &state_stack.back();
		if(curr_state){
			state_stack.pop_back();
		}
		if (!curr_state) {
		    fprintf(out_test, "ERROR: end of block encountered while not inserting!!!\n");
		    break;
		}
		fseek(dxf, curr_state->file_offset, SEEK_SET);
		curr_state->sub_state = UNKNOWN_ENTITY_STATE;
		if (verbose) {
		    fprintf(out_test, "Popped state at end of inserted block (seeked to %jd)\n", (intmax_t)curr_state->file_offset);
		}
		break;
	    } else {
		fprintf(out_test, "Unrecognized entity type encountered (ignoring): %s\n",
		       line);
		break;
	    }
    }
    return 0;
}

static void
insert_init(struct insert_data *ins)
{
    VSETALL(ins->scale, 1.0);
    ins->rotation = 0.0;
    VSETALL(ins->insert_pt, 0.0);
    VSET(ins->extrude_dir, 0, 0, 1);
}


static int
process_insert_entities_code(int code)
{
    static struct insert_data ins;
    static struct state_data *new_state=NULL;
	struct block *blk;
    int coord;

    if (!new_state) {
		insert_init(&ins);
		new_state = new state_data();
		*new_state = *curr_state;
		if (verbose) {
			fprintf(out_test, "Created a new state for INSERT\n");
		}
    }
    switch (code) {
	case 8:		/* layer name */
	    if (curr_layer_name) {
		free(curr_layer_name);
	    }
	    curr_layer_name = make_brlcad_name(line);
	    break;
	case 2:
	    for (int i = 0; i < block_list.size(); i++) {
			if (!strcmp(block_list.at(i).block_name.c_str(), line)) {
				blk = &block_list.at(i);
				new_state->curr_block_indx = i;
				break;
			}
			else{
				blk = nullptr;
			}
	    }
		//BU_LIST_IS_HEAD(blk, &block_head)
	    if (!blk){
			fprintf(out_test, "ERROR: INSERT references non-existent block (%s)\n", line);
			fprintf(out_test, "\tignoring missing block\n");
			blk = NULL;
	    }
	    //new_state->curr_block = blk;
	    if (verbose && blk) {
		fprintf(out_test, "Inserting block %s\n", blk->block_name.c_str());
	    }
	    break;
	case 10:
	case 20:
	case 30:
	    coord = (code / 10) - 1;
	    ins.insert_pt[coord] = atof(line);
	    break;
	case 41:
	case 42:
	case 43:
	    coord = (code % 40) - 1;
	    ins.scale[coord] = atof(line);
	    break;
	case 50:
	    ins.rotation = atof(line);
	    break;
	case 62:	/* color number */
	    curr_color = atoi(line);
	    break;
	case 70:
	case 71:
	    if (atoi(line) != 1) {
			fprintf(out_test, "Cannot yet handle insertion of a pattern\n\tignoring\n");
	    }
	    break;
	case 44:
	case 45:
	    break;
	case 210:
	case 220:
	case 230:
	    coord = ((code / 10) % 20) - 1;
	    ins.extrude_dir[coord] = atof(line);
	    break;
	case 0:		/* end of this insert */

		if (!block_list.at(new_state->curr_block_indx).empty()) {
			double xlate[16], scale[16], rot[16], tmp1[16], tmp2[16];
			MAT_IDN(xlate);
			MAT_IDN(scale);
			MAT_SCALE_VEC(scale, ins.scale);
			MAT_DELTAS_VEC(xlate, ins.insert_pt);
			bn_mat_angles(rot, 0.0, 0.0, ins.rotation);
			bn_mat_mul(tmp1, rot, scale);
			bn_mat_mul(tmp2, xlate, tmp1);
			bn_mat_mul(new_state->xform, tmp2, curr_state->xform);
			
			state_stack.emplace_back(*curr_state);
			curr_state = new_state;
			fseek(dxf, block_list.at(curr_state->curr_block_indx).offset, SEEK_SET);
			curr_state->state = ENTITIES_SECTION;
			curr_state->sub_state = UNKNOWN_ENTITY_STATE;
			new_state = NULL;
			if (verbose) {
				fprintf(stdout, "Changing state for INSERT\n");
				fprintf(stdout, "seeked to %jd\n", (intmax_t)block_list.at(curr_state->curr_block_indx).offset);
				fprintf(stdout, "curr block indx %d \n", curr_state->curr_block_indx);
			}
		}
		break;
    }

    return 0;
}

static int
process_solid_entities_code(int code)
{
    int vert_no;
    int coord;
    double tmp_pt[3];
    struct vertex *v0, *v1;
    static int last_vert_no = -1;
    static double solid_pt[4][3];

    switch (code) {
	case 8:
	    if (curr_layer_name) {
		free(curr_layer_name);
	    }
	    curr_layer_name = make_brlcad_name(line);
	    if (verbose) {
		fprintf(out_test, "LINE is in layer: %s\n", curr_layer_name);
	    }
	    break;
	case 10:
	case 20:
	case 30:
	case 11:
	case 21:
	case 31:
	case 12:
	case 22:
	case 32:
	case 13:
	case 23:
	case 33:
	    vert_no = code % 10;
	    V_MAX(last_vert_no, vert_no);

	    coord = code / 10 - 1;
	    solid_pt[vert_no][coord] = atof(line) * units_conv[units] * scale_factor;
	    if (verbose) {
		fprintf(out_test, "SOLID vertex #%d coord #%d = %g\n", vert_no, coord, solid_pt[vert_no][coord]);
	    }
	    break;
	case 62:	/* color number */
	    curr_color = atoi(line);
	    break;
	case 0:
	    /* end of this solid */
	    get_layer();
	    if (verbose) {
		fprintf(out_test, "Found end of SOLID\n");
	    }
		
	    layers[curr_layer].solid_count++;

	    v0 = NULL;
	    v1 = NULL;
	    for (vert_no = 0; vert_no <= last_vert_no; vert_no ++) {
		MAT4X3PNT(tmp_pt, curr_state->xform, solid_pt[vert_no]);
		VMOVE(solid_pt[vert_no], tmp_pt);

		solid_struct ss; 
		for(int i = 0; i < 4 ; i ++){
			VMOVE(ss.solid_pt[i], solid_pt[i]);
		}
		ss.color = curr_color;
		ss.layer_name = std::string(curr_layer_name);
		dd.solid_vector.emplace_back(ss);
	    }

	    last_vert_no = -1;
	    curr_state->sub_state = UNKNOWN_ENTITY_STATE;
	    process_entities_code[curr_state->sub_state](code);
	    break;
    }

    return 0;
}


static int
process_lwpolyline_entities_code(int code)
{
    double tmp_pt[3];
    static int vert_no = 0;
    static double x, y;

    switch (code) {
	case 8:
	    if (curr_layer_name) {
		free(curr_layer_name);
	    }
	    curr_layer_name = make_brlcad_name(line);
	    if (verbose) {
		fprintf(out_test, "LINE is in layer: %s\n", curr_layer_name);
	    }
	    break;
	case 90:
	    /* oops */
	    break;
	case 10:
	    x = atof(line) * units_conv[units] * scale_factor;
		pt_x.push_back(x);
	    if (verbose) {
		fprintf(out_test, "LWPolyLine vertex #%d (x) = %g\n", vert_no, x);
	    }
	    break;
	case 20:
	{
	    y = atof(line) * units_conv[units] * scale_factor;
		pt_y.push_back(y);
	    if (verbose) {
		fprintf(out_test, "LWPolyLine vertex #%d (y) = %g\n", vert_no, y);
	    }		
	    add_polyline_vertex(x, y, 0.0);
	    break;
	}
	case 62:	/* color number */
	    curr_color = atoi(line);
	    break;
	case 70:
	    polyline_flag = atoi(line);
	    break;
	case 0:
	    /* end of this line */
	    get_layer();

	    if (verbose) {
		fprintf(out_test, "Found end of LWPOLYLINE\n");
	    }
		
	    layers[curr_layer].lwpolyline_count++;

	    if (polyline_vertex_count > 1) {
		struct vertex *v0=NULL, *v1=NULL, *v2=NULL;
		int i;

		for (i = 0; i < polyline_vertex_count; i++) {
		    MAT4X3PNT(tmp_pt, curr_state->xform, &polyline_verts[i*3]);
		    VMOVE(&polyline_verts[i*3], tmp_pt);
		}
		lwpolyline_struct ls; 
		for(int i = 0; i < pt_x.size(); i++){
			ls.lw_pt_vec.push_back(lwpolyline_struct::lw_pt(pt_x.at(i), pt_y.at(i)));
		}

		ls.polyline_flag = polyline_flag;
		ls.color = curr_color;
		ls.layer_name = std::string(curr_layer_name);
		dd.lwpolyline_vector.emplace_back(ls);
		pt_x.clear();
		pt_y.clear();

		// for (i = 0; i < polyline_vertex_count-1; i++) {
		//     struct edgeuse *eu;

		//     eu = nmg_me(v1, v2, layers[curr_layer].s);
		//     if (i == 0) {
		// 	v1 = eu->vu_p->v_p;
		// 	nmg_vertex_gv(v1, polyline_verts);
		// 	v0 = v1;
		//     }
		//     v2 = eu->eumate_p->vu_p->v_p;
		//     nmg_vertex_gv(v2, &polyline_verts[(i+1)*3]);
		//     if (verbose) {
		// 	fprintf(out_test, "Wire edge (lwpolyline): (%g %g %g) <-> (%g %g %g)\n",
		// 	       V3ARGS(v1->vg_p->coord),
		// 	       V3ARGS(v2->vg_p->coord));
		//     }
		//     v1 = v2;
		//     v2 = NULL;
		// }

		// if (polyline_flag & POLY_CLOSED) {
		//     v2 = v0;
		//     (void)nmg_me(v1, v2, layers[curr_layer].s);
		//     if (verbose) {
		// 	fprintf(out_test, "Wire edge (closing lwpolyline): (%g %g %g) <-> (%g %g %g)\n",
		// 	       V3ARGS(v1->vg_p->coord),
		// 	       V3ARGS(v2->vg_p->coord));
		//     }
		// }
	    }
	    polyline_vert_indices_count = 0;
	    polyline_vertex_count = 0;
	    curr_state->sub_state = UNKNOWN_ENTITY_STATE;
	    process_entities_code[curr_state->sub_state](code);
	    break;
    }

    return 0;
}


static int
process_line_entities_code(int code)
{
    int vert_no;
    int coord;
    static double line_pt[2][3];
    struct edgeuse *eu;
    double tmp_pt[3];

    switch (code) {
	case 8:
	    if (curr_layer_name) {
		free(curr_layer_name);
	    }
	    curr_layer_name = make_brlcad_name(line);
	    if (verbose) {
		fprintf(out_test, "LINE is in layer: %s\n", curr_layer_name);
	    }
	    break;
	case 10:
	case 20:
	case 30:
	case 11:
	case 21:
	case 31:
	    vert_no = code % 10;
	    coord = code / 10 - 1;
	    line_pt[vert_no][coord] = atof(line) * units_conv[units] * scale_factor;
	    if (verbose) {
		fprintf(out_test, "LINE vertex #%d coord #%d = %g\n", vert_no, coord, line_pt[vert_no][coord]);
	    }
	    break;
	case 62:	/* color number */
	    curr_color = atoi(line);
	    break;
	case 0:
	    /* end of this line */
	    get_layer();
	    if (verbose) {
		fprintf(out_test, "Found end of LINE\n");
	    }

	    layers[curr_layer].line_count++;

	    MAT4X3PNT(tmp_pt, curr_state->xform, line_pt[0]);
	    VMOVE(line_pt[0], tmp_pt);
	    MAT4X3PNT(tmp_pt, curr_state->xform, line_pt[1]);
	    VMOVE(line_pt[1], tmp_pt);

		line_struct ls; 
		VMOVE(ls.line_pt[0], line_pt[0]);
		VMOVE(ls.line_pt[1], line_pt[1]);
		ls.layer_name = std::string(curr_layer_name);
		ls.color = curr_color;
		dd.line_vector.emplace_back(ls);

	    curr_state->sub_state = UNKNOWN_ENTITY_STATE;
	    process_entities_code[curr_state->sub_state](code);
	    break;
    }

    return 0;
}


static int
process_ellipse_entities_code(int code)
{
    static double center[3]={0, 0, 0};
    static double majorAxis[3]={1.0, 0, 0};
    static double ratio=1.0;
    static double startAngle = 0.0;
    static double endAngle = M_2PI; // means 2 * PI ??
    double angle, delta;
    double majorRadius, minorRadius;
    double tmp_pt[3];
    double xdir[4], ydir[4], zdir[4];
    int coord;
    int fullCircle;
    int done;
    struct vertex *v0 = NULL, *v1 = NULL, *v2 = NULL;
    struct edgeuse *eu;

    switch (code) {
	case 8:		/* layer name */
	    if (curr_layer_name) {
		free(curr_layer_name);
	    }
	    curr_layer_name = make_brlcad_name(line);
	    break;
	case 10:
	case 20:
	case 30:
	    coord = code / 10 - 1;
	    center[coord] = atof(line) * units_conv[units] * scale_factor;
	    break;
	case 11:
	case 21:
	case 31:
	    coord = code / 10 - 1;
	    majorAxis[coord] = atof(line) * units_conv[units] * scale_factor;
	    break;
	case 40:
	    ratio = atof(line);
	    break;
	case 41:
	    startAngle = atof(line);
	    break;
	case 42:
	    endAngle = atof(line);
	    break;
	case 62:	/* color number */
	    curr_color = atoi(line);
	    break;
	case 0:
	    /* end of this ellipse entity
	     * make a series of wire edges in the NMG to approximate a circle
	     */

	    get_layer();
	    if (verbose) {
		fprintf(out_test, "Found an ellipse\n");
	    }

	    layers[curr_layer].ellipse_count++;

	    MAT4X3PNT(tmp_pt, curr_state->xform, center);
	    VMOVE(center, tmp_pt);
	    MAT4X3PNT(tmp_pt, curr_state->xform, majorAxis);
	    VMOVE(majorAxis, tmp_pt);

	    majorRadius = MAGNITUDE(majorAxis);
	    minorRadius = ratio * majorRadius;

	    VMOVE(xdir, majorAxis);
	    VUNITIZE(xdir);
	    VSET(zdir, 0, 0, 1);
	    VCROSS(ydir, zdir, xdir);

		ellipse_struct es; 
		VMOVE(es.center, center);
		es.start_angle = startAngle;
		es.end_angle = endAngle;
		es.layer_name = std::string(curr_layer_name);
		VMOVE(es.majorAxis, majorAxis);
		es.ratio = ratio;
		es.color = curr_color;
		dd.ellipse_vector.emplace_back(es);

	    /* FIXME: arbitrary undefined tolerance */
	    if (NEAR_EQUAL(endAngle, startAngle, 0.001)) {
		fullCircle = 1;
	    } else {
		fullCircle = 0;
	    }

	    if (verbose) {
		fprintf(out_test, "Ellipse:\n");
		fprintf(out_test, "\tcenter = (%g %g %g)\n", V3ARGS(center));
		fprintf(out_test, "\tmajorAxis = (%g %g %g)\n", V3ARGS(majorAxis));
		fprintf(out_test, "\txdir = (%g %g %g)\n", V3ARGS(xdir));
		fprintf(out_test, "\tydir = (%g %g %g)\n", V3ARGS(ydir));
		fprintf(out_test, "\tradii = %g %g\n", majorRadius, minorRadius);
		fprintf(out_test, "\tangles = %g %g\n", startAngle, endAngle);
		fprintf(out_test, "\tfull circle = %d\n", fullCircle);
	    }
	    /* make nmg wire edges */
	    angle = startAngle;
	    delta = M_PI / 15.0; 
	    if ((endAngle - startAngle)/delta < 4) {
		delta = (endAngle - startAngle) / 5.0;
	    }
	    done = 0;
	    while (!done) {
		double p0[3], p1[3];
		double r0, r1;

		if (angle >= endAngle) {
		    angle = endAngle;
		    done = 1;
		}

		r0 = majorRadius * cos(angle);
		r1 = minorRadius * sin(angle);
		VJOIN2(p1, center, r0, xdir, r1, ydir);
		if (angle == startAngle) {//EQUAL(angle, startAngle)
		    VMOVE(p0, p1);
		    angle += delta;
		    continue;
		}
		if (fullCircle && angle == startAngle) {//EQUAL(angle, endAngle)
		    v2 = v0;
		}

		v1 = v2;
		v2 = NULL;

		angle += delta;
	    }

	    VSET(center, 0, 0, 0);
	    VSET(majorAxis, 0, 0, 0);
	    ratio = 1.0;
	    startAngle = 0.0;
	    endAngle = M_2PI;

	    curr_state->sub_state = UNKNOWN_ENTITY_STATE;
	    process_entities_code[curr_state->sub_state](code);
	    break;
    }

    return 0;
}


static int
process_circle_entities_code(int code)
{
    static double center[3];
    static double radius;
    int coord, i;
    struct vertex *v0=NULL, *v1=NULL, *v2=NULL;
    struct edgeuse *eu;

    switch (code) {
	case 8:		/* layer name */
	    if (curr_layer_name) {
		free(curr_layer_name);
	    }
	    curr_layer_name = make_brlcad_name(line);
	    break;
	case 10:
	case 20:
	case 30:
	    coord = code / 10 - 1;
	    center[coord] = atof(line) * units_conv[units] * scale_factor;
	    if (verbose) {
		fprintf(out_test, "CIRCLE center coord #%d = %g\n", coord, center[coord]);
	    }
	    break;
	case 40:
	    radius = atof(line) * units_conv[units] * scale_factor;
	    break;
	case 62:	/* color number */
	    curr_color = atoi(line);
	    break;
	case 0:
	    /* end of this circle entity
	     * make a series of wire edges in the NMG to approximate a circle
	     */

	    get_layer();
	    if (verbose) {
		fprintf(out_test, "Found a circle\n");
	    }

	    layers[curr_layer].circle_count++;

	    /* calculate circle at origin first */
	    VSET(circle_pts[0], radius, 0.0, 0.0);
	    for (i=1; i<segs_per_circle; i++) {
		circle_pts[i][X] = circle_pts[i-1][X]*cos_delta - circle_pts[i-1][Y]*sin_delta;
		circle_pts[i][Y] = circle_pts[i-1][Y]*cos_delta + circle_pts[i-1][X]*sin_delta;
	    }

	    /* move everything to the specified center */
	    for (i = 0; i < segs_per_circle; i++) {
		double tmp_pt[3];
		VADD2(circle_pts[i], circle_pts[i], center);

		/* apply transformation */
		MAT4X3PNT(tmp_pt, curr_state->xform, circle_pts[i]);	
		VMOVE(circle_pts[i], tmp_pt);
	    }

		circle_struct cs; 
		VMOVE(cs.center, center);
		cs.radius = radius;
		cs.layer_name = std::string(curr_layer_name);
		cs.color = curr_color;
		dd.circle_vector.emplace_back(cs);

	    curr_state->sub_state = UNKNOWN_ENTITY_STATE;
	    process_entities_code[curr_state->sub_state](code);
	    break;
    }

    return 0;
}


/* horizontal alignment codes for text */
#define LEFT 0
#define CENTER 1
#define RIGHT 2
#define ALIGNED 3
#define HMIDDLE 4
#define FIT 5

/* vertical alignment codes */
#define BASELINE 0
#define BOTTOM 1
#define VMIDDLE 2
#define TOP 3

/* attachment point codes */
#define TOPLEFT 1
#define TOPCENTER 2
#define TOPRIGHT 3
#define MIDDLELEFT 4
#define MIDDLECENTER 5
#define MIDDLERIGHT 6
#define BOTTOMLEFT 7
#define BOTTOMCENTER 8
#define BOTTOMRIGHT 9

/* secret codes for MTEXT entries
 * \P - new paragraph (line)
 * \X - new line (everything before is above the dimension line, everything after is below)
 * \~ - blank space
 * \f - TrueType font
 * \F - .SHX font
 * ex:
 * Hello \fArial;World
 * or:
 * Hello {\fArial;World}
 */

int
convertSecretCodes(char *c, char *cp, int *maxLineLen)
{
    int lineCount = 0;
    int lineLen = 0;

    while (*c) {
	if (*c == '%' && *(c+1) == '%') {
	    switch (*(c+2)) {
		case 'o':
		case 'O':
		    overstrikemode = !overstrikemode;
		    c += 3;
		    break;
		case 'u':
		case 'U':
		    underscoremode = !underscoremode;
		    c += 3;
		    break;
		case 'd':	/* degree */
		case 'D':
		    *cp++ = 8;
		    c += 3;
		    break;
		case 'p':	 /* plus/minus */
		case 'P':
		    *cp++ = 6;
		    c += 3;
		    break;
		case 'c':	/* diameter */
		case 'C':
		    *cp++ = 7;
		    c += 3;
		    break;
		case '%':
		    *cp++ = '%';
		    c += 3;
		    break;
		default:
		    *cp++ = *c;
		    c++;
		    break;
	    }
	    lineLen++;
	} else if (*c == '\\') {
	    switch (*(c+1)) {
		case 'P':
		case 'X':
		    *cp++ = '\n';
		    c += 2;
		    lineCount++;
		    V_MAX(*maxLineLen, lineLen);

		    lineLen = 0;
		    break;
		case 'A':
		    while (*c != ';' && *c != '\0') c++;
		    c++;
		    break;
		case '~':
		    *cp++ = ' ';
		    c += 2;
		    lineLen++;
		    break;
		default:
		    *cp++ = *c++;
		    lineLen++;
		    break;
	    }
	} else {
	    *cp++ = *c++;
	    lineLen++;
	}
    }
    if (*(c-1) != '\n') {
	lineCount++;
    }

    V_MAX(*maxLineLen, lineLen);

    return lineCount;
}


void
drawString(char *theText, double *firstAlignmentPoint, double *secondAlignmentPoint,
	   double textHeight, double textScale, double textRotation, int horizAlignment, int vertAlignment, int textFlag)
{
    double stringLength = 0.0;
    char *copyOfText;
    char *c, *cp;
    double diff[4];
    std::list<uint32_t> vhead;
    int maxLineLen = 0;
	(void)textScale;
	(void)textFlag;

    //BU_LIST_INIT(&vhead);

    copyOfText = (char *)calloc((unsigned int)strlen(theText)+1, 1); //bu_calloc
    c = theText;
    cp = copyOfText;
    (void)convertSecretCodes(c, cp, &maxLineLen);

    free(theText); //"theText");
    stringLength = strlen(copyOfText);

    if (horizAlignment == FIT && vertAlignment == BASELINE) {
	double allowedLength;
	double xScale=1.0;
	double yScale=1.0;
	double scale;

	/* fit along baseline */
	VSUB2(diff, firstAlignmentPoint, secondAlignmentPoint);
	allowedLength = MAGNITUDE(diff);
	xScale = allowedLength / stringLength;
	yScale = textHeight;
	scale = xScale < yScale ? xScale : yScale;
	// bn_vlist_2string(&vhead, &free_hd, copyOfText,
	// 		 firstAlignmentPoint[X], firstAlignmentPoint[Y],
	// 		 scale, textRotation); 
	// nmg_vlist_to_eu(&vhead, layers[curr_layer].s);
	// BN_FREE_VLIST(&free_hd, &vhead);
    // } else if (horizAlignment == LEFT && vertAlignment == BASELINE) {
	// bn_vlist_2string(&vhead, &free_hd, copyOfText,
	// 		 firstAlignmentPoint[X], firstAlignmentPoint[Y],
	// 		 textHeight, textRotation);
	// nmg_vlist_to_eu(&vhead, layers[curr_layer].s);
	// BN_FREE_VLIST(&free_hd, &vhead);
    // } else if ((horizAlignment == CENTER || horizAlignment == HMIDDLE) && vertAlignment == BASELINE) {
	// double len = stringLength * textHeight;
	// firstAlignmentPoint[X] = secondAlignmentPoint[X] - cos(textRotation) * len / 2.0;
	// firstAlignmentPoint[Y] = secondAlignmentPoint[Y] - sin(textRotation) * len / 2.0;
	// bn_vlist_2string(&vhead, &free_hd, copyOfText,
	// 		 firstAlignmentPoint[X], firstAlignmentPoint[Y],
	// 		 textHeight, textRotation);
	// nmg_vlist_to_eu(&vhead, layers[curr_layer].s);
	// BN_FREE_VLIST(&free_hd, &vhead);
    // } else if ((horizAlignment == CENTER || horizAlignment == HMIDDLE) && vertAlignment == VMIDDLE) {
	// double len = stringLength * textHeight;
	// firstAlignmentPoint[X] = secondAlignmentPoint[X] - len / 2.0;
	// firstAlignmentPoint[Y] = secondAlignmentPoint[Y] - textHeight / 2.0;
	// firstAlignmentPoint[X] = firstAlignmentPoint[X] - (1.0 - cos(textRotation)) * len / 2.0;
	// firstAlignmentPoint[Y] = firstAlignmentPoint[Y] - sin(textRotation) * len / 2.0;
	// bn_vlist_2string(&vhead, &free_hd, copyOfText,
	// 		 firstAlignmentPoint[X], firstAlignmentPoint[Y],
	// 		 textHeight, textRotation);
	// nmg_vlist_to_eu(&vhead, layers[curr_layer].s);
	// BN_FREE_VLIST(&free_hd, &vhead);
    // } else if (horizAlignment == RIGHT && vertAlignment == BASELINE) {
	// double len = stringLength * textHeight;
	// firstAlignmentPoint[X] = secondAlignmentPoint[X] - cos(textRotation) * len;
	// firstAlignmentPoint[Y] = secondAlignmentPoint[Y] - sin(textRotation) * len;
	// bn_vlist_2string(&vhead, &free_hd, copyOfText,
	// 		 firstAlignmentPoint[X], firstAlignmentPoint[Y],
	// 		 textHeight, textRotation);
	// nmg_vlist_to_eu(&vhead, layers[curr_layer].s);
	// BN_FREE_VLIST(&free_hd, &vhead);
    } else {
	fprintf(out_test, "cannot handle this alignment: horiz = %d, vert = %d\n", horizAlignment, vertAlignment);
    }

    free(copyOfText);
}


void
drawMtext(char *text, int attachPoint, int drawingDirection, double textHeight, double entityHeight,
	  double charWidth, double rectWidth, double rotationAngle, double insertionPoint[3])
{
	(void)drawingDirection;
	(void)rectWidth;
	//Removed UNUSED()
    std::list<uint32_t> vhead;
    int done;
    char *c;
    char *cp;
    int lineCount;
    double lineSpace;
    double xdir[4], ydir[4];
    double startx, starty;
    int maxLineLen = 0;
    double scale = 1.0;
    double xdel = 0.0, ydel = 0.0;
    double radians = rotationAngle * DEG2RAD;
    char *copyOfText = (char *)calloc((unsigned int)strlen(text)+1, 1);

    //BU_LIST_INIT(&vhead);

    c = text;
    cp = copyOfText;
    lineCount = convertSecretCodes(c, cp, &maxLineLen);

    if (textHeight > 0.0) {
	scale = textHeight;
    } else if (charWidth > 0.0) {
	scale = charWidth;
    } else if (entityHeight > 0.0) {
	scale = (entityHeight / (double)lineCount) * 0.9;
    }

    lineSpace = 1.25 * scale;

    VSET(xdir, cos(radians), sin(radians), 0.0);
    VSET(ydir, -sin(radians), cos(radians), 0.0);

    switch (attachPoint) {
	case TOPLEFT:
	    xdel = 0.0;
	    ydel =  -scale;
	    break;
	case TOPCENTER:
	    xdel = -((double)maxLineLen * scale) / 2.0;
	    ydel = -scale;
	    break;
	case TOPRIGHT:
	    xdel = -(double)maxLineLen * scale;
	    ydel = -scale;
	    break;
	case MIDDLELEFT:
	    xdel = 0.0;
	    ydel = -((double)lineCount * lineSpace) / 2.0;
	    break;
	case MIDDLECENTER:
	    xdel = -((double)maxLineLen * scale) / 2.0;
	    ydel = -((double)lineCount * lineSpace) / 2.0;
	    break;
	case MIDDLERIGHT:
	    xdel = -(double)maxLineLen * scale;
	    ydel = -((double)lineCount * lineSpace) / 2.0;
	    break;
	case BOTTOMLEFT:
	    xdel = 0.0;
	    ydel = (double)lineCount * lineSpace - scale;
	    break;
	case BOTTOMCENTER:
	    xdel = -((double)maxLineLen * scale) / 2.0;
	    ydel = (double)lineCount * lineSpace - scale;
	    break;
	case BOTTOMRIGHT:
	    xdel = -(double)maxLineLen * scale;
	    ydel = (double)lineCount * lineSpace - scale;
	    break;
    }

    startx = insertionPoint[X] + xdel * xdir[X] + ydel * ydir[X];
    starty = insertionPoint[Y] + xdel * xdir[Y] + ydel * ydir[Y];

    cp = copyOfText;
    c = copyOfText;
    done = 0;
    while (!done) {
	if (*cp == '\n' || *cp == '\0') {
	    if (*cp == '\0') {
		done = 1;
	    }
	    *cp = '\0';
	    // bn_vlist_2string(&vhead, &free_hd, c,
		// 	     startx, starty,
		// 	     scale, rotationAngle);
	    // nmg_vlist_to_eu(&vhead, layers[curr_layer].s);
	    // BN_FREE_VLIST(&free_hd, &vhead);
	    c = ++cp;
	    startx -= lineSpace * ydir[X];
	    starty -= lineSpace * ydir[Y];
	} else {
	    cp++;
	}
    }


    free(copyOfText); //"copyOfText");
}


static int
process_leader_entities_code(int code)
{
    static int arrowHeadFlag = 0;
    static int vertNo = 0;
    static double pt[3];
    double tmp_pt[3];
    int i;
    struct edgeuse *eu;
    struct vertex *v1=NULL, *v2=NULL;

    switch (code) {
	case 8:
	    if (curr_layer_name) {
		free(curr_layer_name);
	    }
	    curr_layer_name = make_brlcad_name(line);
	    if (verbose) {
		fprintf(out_test, "LINE is in layer: %s\n", curr_layer_name);
	    }
	    break;
	case 62:	/* color number */
	    curr_color = atoi(line);
	    break;
	case 71:
	    arrowHeadFlag = atoi(line);
	    break;
	case 72:
	    /* path type, unimplemented */
	    break;
	case 73:
	    /* creation, unimplemented */
	    break;
	case 74:
	    /* hookline direction, unimplemented */
	    break;
	case 75:
	    /* hookline, unimplemented */
	    break;
	case 40:
	    /* text height, unimplemented */
	    break;
	case 41:
	    /* text width, unimplemented */
	    break;
	case 76:
	    /* num vertices, unimplemented */
	    break;
	case 210:
	    /* normal, unimplemented */
	    break;
	case 220:
	    /* normal, unimplemented */
	    break;
	case 230:
	    /* normal, unimplemented */
	    break;
	case 211:
	    /* horizontal direction, unimplemented */
	    break;
	case 221:
	    /* horizontal direction, unimplemented */
	    break;
	case 231:
	    /* horizontal direction, unimplemented */
	    break;
	case 212:
	    /* offsetB, unimplemented */
	    break;
	case 222:
	    /* offsetB, unimplemented */
	    break;
	case 232:
	    /* offsetB, unimplemented */
	    break;
	case 213:
	    /* offset, unimplemented */
	    break;
	case 223:
	    /* offset, unimplemented */
	    break;
	case 233:
	    /* offset, unimplemented */
	    break;
	case 10:
	    pt[X] = atof(line);
	    break;
	case 20:
	    pt[Y] = atof(line);
	    break;
	case 30:
	    pt[Z] = atof(line);
	    if (verbose) {
		fprintf(out_test, "LEADER vertex #%d = (%g %g %g)\n", vertNo, V3ARGS(pt));
	    }
	    MAT4X3PNT(tmp_pt, curr_state->xform, pt);
		pt_x.push_back(tmp_pt[X]);
		pt_y.push_back(tmp_pt[Y]);
		pt_z.push_back(tmp_pt[Z]);
	    add_polyline_vertex(V3ARGS(tmp_pt));
	    break;
	case 0:
	    /* end of this line */
	    get_layer();
	    if (verbose) {
		fprintf(out_test, "Found end of LEADER: arrowhead flag = %d\n", arrowHeadFlag);
	    }

		leader_struct ls;
		ls.pt[0] = pt_x.at(0);
		ls.pt[1] = pt_y.at(0);
		ls.pt[2] = pt_z.at(0);
		ls.arrrowHeadFlag = arrowHeadFlag;
		ls.color = curr_color;
		ls.layer_name = std::string(curr_layer_name);
		dd.leader_vector.emplace_back(ls);
		pt_x.clear();
		pt_y.clear();
		pt_z.clear();

	    layers[curr_layer].leader_count++;

	    // if (polyline_vertex_count > 1) {
		// if (!layers[curr_layer].m) {
		//     create_nmg();
		// }

		// for (i = 0; i < polyline_vertex_count-1; i++) {
		//     eu = nmg_me(v1, v2, layers[curr_layer].s);
		//     if (i == 0) {
		// 	v1 = eu->vu_p->v_p;
		// 	nmg_vertex_gv(v1, polyline_verts);
		//     }
		//     v2 = eu->eumate_p->vu_p->v_p;
		//     nmg_vertex_gv(v2, &polyline_verts[(i+1)*3]);
		//     if (verbose) {
		// 	fprintf(out_test, "Wire edge (LEADER): (%g %g %g) <-> (%g %g %g)\n",
		// 	       V3ARGS(v1->vg_p->coord),
		// 	       V3ARGS(v2->vg_p->coord));
		//     }
		//     v1 = v2;
		//     v2 = NULL;
		// }
	    // }
	    polyline_vert_indices_count = 0;
	    polyline_vertex_count = 0;
	    arrowHeadFlag = 0;
	    vertNo = 0;

	    curr_state->sub_state = UNKNOWN_ENTITY_STATE;
	    process_entities_code[curr_state->sub_state](code);
	    break;
    }

    return 0;
}


static int
process_mtext_entities_code(int code)
{
    std::string vls;
    static int attachPoint = 0;
    static int drawingDirection = 0;
    static double textHeight = 0.0;
    static double entityHeight = 0.0;
    static double charWidth = 0.0;
    static double rectWidth = 0.0;
    static double rotationAngle = 0.0;
    static double insertionPoint[3] = {0, 0, 0};
    static double xAxisDirection[3] = {0, 0, 0};
    double tmp_pt[3];
    int coord;

    switch (code) {
	case 3:
	    if (vls.empty()) {
		//BU_GET(vls);
		//bu_vls_init(vls);
	    }
		vls += Char2String(line);
	    break;
	case 1:
	    if (vls.empty()) {	
		//BU_GET(vls);
		//bu_vls_init(vls);
	    }
		vls += Char2String(line);
	    break;
	case 8:		/* layer name */
	    if (curr_layer_name) {
		free(curr_layer_name);
	    }
	    curr_layer_name = make_brlcad_name(line);
	    break;
	case 10:
	case 20:
	case 30:
	    coord = (code / 10) - 1;
	    insertionPoint[coord] = atof(line) * units_conv[units] * scale_factor;
	    break;
	case 11:
	case 21:
	case 31:
	    coord = (code / 10) - 1;
	    xAxisDirection[coord] = atof(line);
	    if (code == 31) {
		rotationAngle = atan2(xAxisDirection[Y], xAxisDirection[X]) * RAD2DEG;
	    }
	    break;
	case 40:
	    textHeight = atof(line);
	    break;
	case 41:
	    rectWidth = atof(line);
	    break;
	case 42:
	    charWidth = atof(line);
	    break;
	case 43:
	    entityHeight = atof(line);
	    break;
	case 50:
	    rotationAngle = atof(line);
	    break;
	case 62:	/* color number */
	    curr_color = atoi(line);
	    break;
	case 71:
	    attachPoint = atoi(line);
	    break;
	case 72:
	    drawingDirection = atoi(line);
	    break;
	case 0:
	    if (verbose) {
		fprintf(out_test, "MTEXT (%s), height = %g, entityHeight = %g, rectWidth = %g\n", (!vls.empty()) ? vls.c_str() : "NO_NAME", textHeight, entityHeight, rectWidth);//(vls) ? bu_vls_addr(vls)
		fprintf(out_test, "\tattachPoint = %d, charWidth = %g, insertPt = (%g %g %g)\n", attachPoint, charWidth, V3ARGS(insertionPoint));
	    }
	    /* draw the text */
	    get_layer();

		mtext_struct ms;
		ms.attachPoint = attachPoint;
		ms.charWidth = charWidth;
		ms.drawingDirection = drawingDirection;
		ms.entityHeight = entityHeight;
		ms.rectWidth = rectWidth;
		ms.rotationAngle = rotationAngle;
		ms.textHeight = textHeight;
		ms.vls = vls;
		VMOVE(ms.xAxisDirection, xAxisDirection);
		VMOVE(ms.insertionPoint, insertionPoint);
		ms.layer_name = std::string(curr_layer_name);
		ms.color = curr_color;
		dd.mtext_vector.emplace_back(ms);

	    layers[curr_layer].mtext_count++;

	    /* apply transformation */
	    MAT4X3PNT(tmp_pt, curr_state->xform, insertionPoint);
	    VMOVE(insertionPoint, tmp_pt);

	    {
		char noname[] = "NO_NAME";
		char *t = NULL;
		if (!vls.empty()) {
			t = strdup(vls.c_str());
		}
		drawMtext((t) ? t : noname, attachPoint, drawingDirection, textHeight, entityHeight,
			  charWidth, rectWidth, rotationAngle, insertionPoint);
		if (t)
		    free(t); //"temp char buf");
	    }
	    // BU_PUT(vls, struct bu_vls);

	    attachPoint = 0;
	    textHeight = 0.0;
	    entityHeight = 0.0;
	    charWidth = 0.0;
	    rectWidth = 0.0;
	    rotationAngle = 0.0;
	    VSET(insertionPoint, 0, 0, 0);
	    VSET(xAxisDirection, 0, 0, 0);
	    curr_state->sub_state = UNKNOWN_ENTITY_STATE;
	    process_entities_code[curr_state->sub_state](code);
	    break;
    }

    return 0;
}


static int
process_text_attrib_entities_code(int code)
{
    /* Secret text code used in DXF files:
     *
     * %%o - toggle overstrike mode
     * %%u - toggle underscore mode
     * %%d - degree symbol
     * %%p - tolerance symbol (plus/minus)
     * %%c - diameter symbol (circle with line through it, lower left to upper right
     * %%% - percent symbol
     */

    static char *theText=NULL;
    static int horizAlignment = 0;
    static int vertAlignment = 0;
    static int textFlag = 0;
    static double firstAlignmentPoint[3] = {0, 0, 0};//VINIT_ZERO;
    static double secondAlignmentPoint[3] = {0, 0, 0};//VINIT_ZERO;
    static double textScale = 1.0;
    static double textHeight;
    static double textRotation = 0.0;
    double tmp_pt[3];
    int coord;

    switch (code) {
	case 1:
	    theText = strdup(line);
	    break;
	case 8:		/* layer name */
	    if (curr_layer_name) {
		free(curr_layer_name);
	    }
	    curr_layer_name = make_brlcad_name(line);
	    break;
	case 10:
	case 20:
	case 30:
	    coord = (code / 10) - 1;
	    firstAlignmentPoint[coord] = atof(line) * units_conv[units] * scale_factor;
	    break;
	case 11:
	case 21:
	case 31:
	    coord = (code / 10) - 1;
	    secondAlignmentPoint[coord] = atof(line) * units_conv[units] * scale_factor;
	    break;
	case 40:
	    textHeight = atof(line);
	    break;
	case 41:
	    textScale = atof(line);
	    break;
	case 50:
	    textRotation = atof(line);
	    break;
	case 62:	/* color number */
	    curr_color = atoi(line);
	    break;
	case 71:
	    textFlag = atoi(line);
	    break;
	case 72:
	    horizAlignment = atoi(line);
	    break;
	case 73:
	    vertAlignment = atoi(line);
	    break;
	case 0:
	    if (theText != NULL) {
		if (verbose) {
		    fprintf(out_test, "TEXT (%s), height = %g, scale = %g\n", theText, textHeight, textScale);
		}
		/* draw the text */
		get_layer();

		text_attrib_struct tas;
		tas.color = curr_color;
		VMOVE(tas.firstAlignmentPoint, firstAlignmentPoint);
		tas.horizAlignment = horizAlignment;
		VMOVE(tas.secondAlignmentPoint, secondAlignmentPoint);
		tas.textFlag = textFlag;
		tas.textHeight = textHeight;
		tas.textRotation = textRotation;
		tas.textScale = textScale;
		tas.theText = std::string(theText);
		tas.vertAlignment = vertAlignment;
		dd.text_attrib_vector.emplace_back(tas);

		/* apply transformation */
		MAT4X3PNT(tmp_pt, curr_state->xform, firstAlignmentPoint);
		VMOVE(firstAlignmentPoint, tmp_pt);
		MAT4X3PNT(tmp_pt, curr_state->xform, secondAlignmentPoint);
		VMOVE(secondAlignmentPoint, tmp_pt);

		drawString(theText, firstAlignmentPoint, secondAlignmentPoint,
			   textHeight, textScale, textRotation, horizAlignment, vertAlignment, textFlag);
		layers[curr_layer].text_count++;
	    }
	    horizAlignment = 0;
	    vertAlignment = 0;
	    textFlag = 0;
	    VSET(firstAlignmentPoint, 0.0, 0.0, 0.0);
	    VSET(secondAlignmentPoint, 0.0, 0.0, 0.0);
	    textScale = 1.0;
	    textRotation = 0.0;
	    curr_state->sub_state = UNKNOWN_ENTITY_STATE;
	    process_entities_code[curr_state->sub_state](code);
	    break;
    }

    return 0;
}


static int
process_dimension_entities_code(int code)
{
    static char *block_name=NULL;
    static struct state_data *new_state=NULL;
    struct block *blk;

    switch (code) {
	case 10:
	case 20:
	case 30:
	    break;
	case 8:		/* layer name */
	    if (curr_layer_name) {
		free(curr_layer_name);
	    }
	    curr_layer_name = make_brlcad_name(line);
	    break;
	case 2:	/* block name */
	    block_name = strdup(line); //what to replace ??
	    break;
	case 0:
	    if (block_name != NULL) {
		/* insert this dimension block */
		get_layer();

		dimension_struct ds;
		ds.block_name = std::string(block_name);
		ds.layer_name = std::string(curr_layer_name);
		dd.dimension_vector.emplace_back(ds);

		new_state = new state_data(); // bu_alloc
		*new_state = *curr_state;
		if (verbose) {
		    fprintf(out_test, "Created a new state for DIMENSION\n");
		}
		//BU_LIST_FOR(blk, block_list, &block_head)
		for (int i = 0; i < block_list.size(); i++) {
			if (strcmp(block_list.at(i).block_name.c_str(), line)) {
				blk = &block_list.at(i);
				new_state->curr_block_indx = i;
				break;
			}
	    }
		if (!blk) {
		    fprintf(out_test, "ERROR: DIMENSION references non-existent block (%s)\n", block_name);
		    fprintf(out_test, "\tignoring missing block\n");
		    blk = NULL;
		}
		//new_state->curr_block = &(*blk);
		if (verbose && blk) {
		    fprintf(out_test, "Inserting block %s\n", blk->block_name.c_str());
		}

		if (block_name) {
		    delete block_name;// "block_name");
		}

		if (!block_list.at(new_state->curr_block_indx).empty()) {
		    //BU_LIST_PUSH(&state_stack, &(curr_state->l)); place the item at the tail of the list
			state_stack.emplace_back(*curr_state);
		    curr_state = new_state;
		    delete new_state;
			new_state = NULL;
		    fseek(dxf,  block_list.at(curr_state->curr_block_indx).offset, SEEK_SET);
		    curr_state->state = ENTITIES_SECTION;
		    curr_state->sub_state = UNKNOWN_ENTITY_STATE;
		    if (verbose) {
			fprintf(out_test, "Changing state for INSERT\n");
			fprintf(out_test, "seeked to %jd\n", (intmax_t) block_list.at(curr_state->curr_block_indx).offset);
		    }
		    layers[curr_layer].dimension_count++;
		}
	    } else {
		curr_state->sub_state = UNKNOWN_ENTITY_STATE;
		process_entities_code[curr_state->sub_state](code);
	    }
	    break;
    }

    return 0;
}


static int
process_arc_entities_code(int code)
{
    static double center[3]={0, 0, 0};
    static double radius;
    static double start_angle, end_angle;
    int num_segs;
    int coord, i;
    struct vertex *v0=NULL, *v1=NULL, *v2=NULL;
    struct edgeuse *eu;

    switch (code) {
	case 8:		/* layer name */
	    if (curr_layer_name) {
		free(curr_layer_name);
	    }
	    curr_layer_name = make_brlcad_name(line);
	    break;
	case 10:
	case 20:
	case 30:
	    coord = code / 10 - 1;
	    center[coord] = atof(line) * units_conv[units] * scale_factor;
	    if (verbose) {
		fprintf(out_test, "ARC center coord #%d = %g\n", coord, center[coord]);
	    }
	    break;
	case 40:
	    radius = atof(line) * units_conv[units] * scale_factor;
	    if (verbose) {
		fprintf(out_test, "ARC radius = %g\n", radius);
	    }
	    break;
	case 50:
	    start_angle = atof(line);
	    if (verbose) {
		fprintf(out_test, "ARC start angle = %g\n", start_angle);
	    }
	    break;
	case 51:
	    end_angle = atof(line);
	    if (verbose) {
		fprintf(out_test, "ARC end angle = %g\n", end_angle);
	    }
	    break;
	case 62:	/* color number */
	    curr_color = atoi(line);
	    break;
	case 0:
	    /* end of this arc entity
	     * make a series of wire edges in the NMG to approximate an arc
	     */

	    get_layer();
	    if (verbose) {
		fprintf(out_test, "Found an arc\n");
	    }

		arc_struct as;
		VMOVE(as.center, center);
		as.radius = radius;
		as.start_angle = start_angle;
		as.end_angle = end_angle;
		as.color = curr_color;
		as.layer_name = std::string(curr_layer_name);
		dd.arc_vector.emplace_back(as);

	    layers[curr_layer].arc_count++;

	    while (end_angle < start_angle) {
		end_angle += 360.0;
	    }

	    /* calculate arc at origin first */
	    num_segs = (end_angle - start_angle) / 360.0 * segs_per_circle;
		start_angle *= DEG2RAD;
	    end_angle *= DEG2RAD;
	    if (verbose) {
			fprintf(out_test, "arc has %d segs\n", num_segs);
	    }

	    V_MAX(num_segs, 1);

	    VSET(circle_pts[0], radius * cos(start_angle), radius * sin(start_angle), 0.0);
	    for (i=1; i<num_segs; i++) {
		circle_pts[i][X] = circle_pts[i-1][X]*cos_delta - circle_pts[i-1][Y]*sin_delta;
		circle_pts[i][Y] = circle_pts[i-1][Y]*cos_delta + circle_pts[i-1][X]*sin_delta;
	    }
	    circle_pts[num_segs][X] = radius * cos(end_angle);
	    circle_pts[num_segs][Y] = radius * sin(end_angle);
	    num_segs++;

	    if (verbose) {
		fprintf(out_test, "ARC points calculated:\n");
		for (i = 0; i < num_segs; i++) {
		    fprintf(out_test, "\t point #%d: (%g %g %g)\n", i, V3ARGS(circle_pts[i]));
		}
	    }

	    /* move everything to the specified center */
	    for (i = 0; i < num_segs; i++) {
		double tmp_pt[3];

		VADD2(circle_pts[i], circle_pts[i], center);

		/* apply transformation */
		MAT4X3PNT(tmp_pt, curr_state->xform, circle_pts[i]);
		VMOVE(circle_pts[i], tmp_pt);
	    }

	    if (verbose) {
		fprintf(out_test, "ARC points after move to center at (%g %g %g):\n", V3ARGS(center));
		for (i = 0; i < num_segs; i++) {
		    fprintf(out_test, "\t point #%d: (%g %g %g)\n", i, V3ARGS(circle_pts[i]));
		}
	    }

	    /* make nmg wire edges */
	    // for (i = 1; i < num_segs; i++) {
		// if (i == num_segs) {
		//     v2 = v0;
		// }
		// eu = nmg_me(v1, v2, layers[curr_layer].s);
		// if (i == 1) {
		//     v1 = eu->vu_p->v_p;
		//     v0 = v1;
		//     nmg_vertex_gv(v1, circle_pts[0]);
		// }
		// v2 = eu->eumate_p->vu_p->v_p;
		// if (i < segs_per_circle) {
		//     nmg_vertex_gv(v2, circle_pts[i]);
		// }
		// if (verbose) {
		//     fprintf(out_test, "Wire edge (arc) #%d (%g %g %g) <-> (%g %g %g)\n", i,
		// 	   V3ARGS(v1->vg_p->coord),
		// 	   V3ARGS(v2->vg_p->coord));
		// }
		// v1 = v2;
		// v2 = NULL;
	    // }

	    VSETALL(center, 0.0);
	    for (i = 0; i < segs_per_circle; i++) {
		VSETALL(circle_pts[i], 0.0);
	    }

	    curr_state->sub_state = UNKNOWN_ENTITY_STATE;
	    process_entities_code[curr_state->sub_state](code);
	    break;
    }

    return 0;
}


static int
process_spline_entities_code(int code)
{
    static int flag = 0;
    static int degree = 0;
    static int numKnots = 0;
    static int numCtlPts = 0;
    static int numFitPts = 0;
    static double *knots = NULL;
    static double *weights = NULL;
    static double *ctlPts = NULL;
    static double *fitPts = NULL;
    static int knotCount = 0;
    static int weightCount = 0;
    static int ctlPtCount = 0;
    static int fitPtCount = 0;
    static int subCounter = 0;
    static int subCounter2 = 0;
    int i;
    int coord;
    //struct edge_g_cnurb *crv;
    int pt_type;
    int ncoords;
    struct vertex *v1 = NULL;
    struct vertex *v2 = NULL;
    struct edgeuse *eu;
    double startParam;
    double stopParam;
    double paramDelta;
    double pt[3];

    switch (code) {
	case 8:
	    if (curr_layer_name) {
		free(curr_layer_name);
	    }
	    curr_layer_name = make_brlcad_name(line);
	    break;
	case 210:
	case 220:
	case 230:
	    coord = code / 10 - 21;
	    break;
	case 70:
	    flag = atoi(line);
	    break;
	case 71:
	    degree = atoi(line);
	    break;
	case 72:
	    numKnots = atoi(line);
	    if (numKnots > 0) {
		knots = (double *)malloc(numKnots*sizeof(double));
	    }
	    break;
	case 73:
	    numCtlPts = atoi(line);
	    if (numCtlPts > 0) {
		ctlPts = (double *)malloc(numCtlPts*3*sizeof(double));
					      //"spline control points");
		weights = (double *)malloc(numCtlPts*sizeof(double));
					       //"spline weights");
	    }
	    for (i = 0; i < numCtlPts; i++) {
		weights[i] = 1.0;
	    }
	    break;
	case 74:
	    numFitPts = atoi(line);
	    if (numFitPts > 0) {
		fitPts = (double *)malloc(numFitPts*3*sizeof(double));
					      //"fit control points");
	    }
	    break;
	case 42:
	    break;
	case 43:
	    break;
	case 44:
	    break;
	case 12:
	case 22:
	case 32:
	    coord = code / 10 - 1;
	    /* start tangent, unimplemented */
	    break;
	case 13:
	case 23:
	case 33:
	    coord = code / 10 - 1;
	    /* end tangent, unimplemented */
	    break;
	case 40:
	    knots[knotCount++] = atof(line);
	    break;
	case 41:
	    weights[weightCount++] = atof(line);
	    break;
	case 10:
	case 20:
	case 30:
	    coord = (code / 10) - 1 + ctlPtCount*3;
	    ctlPts[coord] = atof(line) * units_conv[units] * scale_factor;
	    subCounter++;
	    if (subCounter > 2) {
		ctlPtCount++;
		subCounter = 0;
	    }
	    break;
	case 11:
	case 21:
	case 31:
	    coord = (code / 10) - 1 + fitPtCount*3;
	    fitPts[coord] = atof(line) * units_conv[units] * scale_factor;
	    subCounter2++;
	    if (subCounter2 > 2) {
		fitPtCount++;
		subCounter2 = 0;
	    }
	    break;
	case 62:	/* color number */
	    curr_color = atoi(line);
	    break;
	case 0:
	    /* draw the spline */
	    get_layer();

		spline_struct ss;
		ss.flag = flag;
		ss.degree = degree;
		ss.numKnots = numKnots;
		ss.numCtlPts = numCtlPts;
		ss.numFitPts = numFitPts;
		ss.color = curr_color;
		ss.layer_name = std::string(curr_layer_name);
		for(int i = 0; i < numKnots; i++){
			ss.knots.emplace_back(knots[i]);
		}
		for(int i = 0; i < numCtlPts; i++){
			spline_struct::spline_pts sPts;
			sPts.weights = weights[i];
			VSET(sPts.spoints, ctlPts[i*3+0], ctlPts[i*3+1], ctlPts[i*3+2]);
			ss.ctlPts.emplace_back(sPts);
		}
		for(int i = 0; i < numFitPts; i++){
			spline_struct::spline_pts sPts;
			VSET(sPts.spoints, fitPts[i*3+0], fitPts[i*3+1], fitPts[i*3+2]);
			ss.ctlPts.emplace_back(sPts);
		}
		dd.spline_vector.emplace_back(ss);

	    layers[curr_layer].spline_count++;

	    if (flag & SPLINE_RATIONAL) {
		ncoords = 4;
		//pt_type = RT_NURB_MAKE_PT_TYPE(ncoords, RT_NURB_PT_XYZ, RT_NURB_PT_RATIONAL);
	    } else {
		ncoords = 3;
		//pt_type = RT_NURB_MAKE_PT_TYPE(ncoords, RT_NURB_PT_XYZ, RT_NURB_PT_NONRAT);
	    }
	    // crv = nmg_nurb_new_cnurb(degree+1, numCtlPts+degree+1, numCtlPts, pt_type);

	    // for (i = 0; i < numKnots; i++) {
		// crv->k.knots[i] = knots[i];
	    // }
	    // for (i = 0; i < numCtlPts; i++) {
		// crv->ctl_points[i*ncoords + 0] = ctlPts[i*3+0];
		// crv->ctl_points[i*ncoords + 1] = ctlPts[i*3+1];
		// crv->ctl_points[i*ncoords + 2] = ctlPts[i*3+2];
		// if (flag & SPLINE_RATIONAL) {
		//     crv->ctl_points[i*ncoords + 3] = weights[i];
		// }
	    // }
	    // if (!layers[curr_layer].m) {
		// create_nmg();
	    // }
	    // startParam = knots[0];
	    // stopParam = knots[numKnots-1];
	    // paramDelta = (stopParam - startParam) / (double)splineSegs;
	    // nmg_nurb_c_eval(crv, startParam, pt);
	    // for (i = 0; i < splineSegs; i++) {
		// double param = startParam + paramDelta * (i+1);
		// eu = nmg_me(v1, v2, layers[curr_layer].s);
		// v1 = eu->vu_p->v_p;
		// if (i == 0) {
		//     nmg_vertex_gv(v1, pt);
		// }
		// nmg_nurb_c_eval(crv, param, pt);
		// v2 = eu->eumate_p->vu_p->v_p;
		// nmg_vertex_gv(v2, pt);

		// v1 = v2;
		// v2 = NULL;
	    // }

	    // nmg_nurb_free_cnurb(crv);

	    if (knots != NULL) free(knots); //"spline knots");
	    if (weights != NULL) free(weights);// "spline weights");
	    if (ctlPts != NULL) free(ctlPts); //"spline control points");
	    if (fitPts != NULL) free(fitPts); //"spline fit points");
	    flag = 0;
	    degree = 0;
	    numKnots = 0;
	    numCtlPts = 0;
	    numFitPts = 0;
	    knotCount = 0;
	    weightCount = 0;
	    ctlPtCount = 0;
	    fitPtCount = 0;
	    subCounter = 0;
	    subCounter2 = 0;
	    knots = NULL;
	    weights = NULL;
	    ctlPts = NULL;
	    fitPts = NULL;

	    curr_state->sub_state = UNKNOWN_ENTITY_STATE;
	    process_entities_code[curr_state->sub_state](code);
	    break;
    }

    return 0;
}
static int
process_3dface_entities_code(int code)
{
    int vert_no;
    int coord;
    int face[5];

    switch (code) {
	case 8:
	    if (curr_layer_name) {
		free(curr_layer_name);
	    }
	    curr_layer_name = make_brlcad_name(line);
	    break;
	case 10:
	case 20:
	case 30:
	case 11:
	case 21:
	case 31:
	case 12:
	case 22:
	case 32:
	case 13:
	case 23:
	case 33:
	    vert_no = code % 10;
	    coord = code / 10 - 1;
	    pts[vert_no][coord] = atof(line) * units_conv[units] * scale_factor;
	    if (verbose) {
		fprintf(out_test, "3dface vertex #%d coord #%d = %g\n", vert_no, coord, pts[vert_no][coord]);
	    }
	    if (vert_no == 2) {
		pts[3][coord] = pts[2][coord];
	    }
	    break;
	case 62:	/* color number */
	    curr_color = atoi(line);
	    break;
	case 0:
	    /* end of this 3dface */
	    get_layer();
	    if (verbose) {
		fprintf(out_test, "Found end of 3DFACE\n");
	    }
	    if (verbose) {
		fprintf(out_test, "\tmaking two triangles\n");
	    }

		face3d_struct f3d;
		for(int i = 0; i < 4; i++){
			VMOVE(f3d.pts[i], pts[i]);
		}
		f3d.color = curr_color;
		f3d.layer_name = std::string(curr_layer_name);
		dd.face3d_vector.emplace_back(f3d);
		
	    layers[curr_layer].face3d_count++;
	    for (vert_no = 0; vert_no < 4; vert_no++) {
		double tmp_pt1[3];
		MAT4X3PNT(tmp_pt1, curr_state->xform, pts[vert_no]);
		VMOVE(pts[vert_no], tmp_pt1);
		face[vert_no] = Add_vert(V3ARGS(pts[vert_no]),
						 layers[curr_layer].vert_tree,
						 tol_sq);		
	    }
	    add_triangle(face[0], face[1], face[2], curr_layer);
	    add_triangle(face[2], face[3], face[0], curr_layer);
	    if (verbose) {
		fprintf(out_test, "finished face\n");
	    }

	    curr_state->sub_state = UNKNOWN_ENTITY_STATE;
	    process_entities_code[curr_state->sub_state](code);
	    break;
    }

    return 0;
}


static int
process_entity_code(int code)
{
    return process_entities_code[curr_state->sub_state](code);
}


static int
process_objects_code(int code)
{
    switch (code) {
	case 999:	/* comment */
	    printf("%s\n", line);
	    break;
	case 0:		/* text string */
	    if (!strncmp(line, "SECTION", 7)) {
		curr_state->state = UNKNOWN_SECTION;
		break;
	    } else if (!strncmp(line, "ENDSEC", 6)) {
		curr_state->state = UNKNOWN_SECTION;
		break;
	    }
	    break;
    }

    return 0;
}


static int
process_thumbnail_code(int code)
{
    switch (code) {
	case 999:	/* comment */
	    printf("%s\n", line);
	    break;
	case 0:		/* text string */
	    if (!strncmp(line, "SECTION", 7)) {
		curr_state->state = UNKNOWN_SECTION;
		break;
	    } else if (!strncmp(line, "ENDSEC", 6)) {
		curr_state->state = UNKNOWN_SECTION;
		break;
	    }
	    break;
    }

    return 0;
}

int
readcodes()
{
    int code;
    size_t line_len;
    static int line_num = 0;

    curr_state->file_offset = ftell(dxf);

    if (fgets(line, MAX_LINE_SIZE, dxf) == NULL) {
	return ERROR_FLAG;
    } else {
	code = atoi(line);
    }

    if (fgets(line, MAX_LINE_SIZE, dxf) == NULL) {
	return ERROR_FLAG;
    }

    if (!strncmp(line, "EOF", 3)) {
	return EOF_FLAG;
    }

    line_len = strlen(line);
    if (line_len) {
	line[line_len-1] = '\0';
	line_len--;
    }

    if (line_len && line[line_len-1] == '\r') {
	line[line_len-1] = '\0';
	line_len--;
    }

    if (verbose) {
	line_num++;
	fprintf(out_test, "%d:\t%d\n", line_num, code);
	line_num++;
	fprintf(out_test, "%d:\t%s\n", line_num, line);
    }

    return code;
}

std::vector<circle_struct> dxf_data::return_circle_vector(){
	return circle_vector; 
}

std::vector<polyline_vertex_struct> dxf_data::return_polyline_vertex_vector(){
	return polyline_vertex_vector;
}

std::vector<polyline_struct> dxf_data::return_polyline_vector(){
	return polyline_vector;
}

std::vector<lwpolyline_struct> dxf_data::return_lwpolyline_vector(){
	return lwpolyline_vector;
}

std::vector<face3d_struct> dxf_data::return_face3d_vector(){
	return face3d_vector;
}

std::vector<line_struct> dxf_data::return_line_vector(){
	return line_vector;
}

std::vector<point_struct> dxf_data::return_point_vector(){
	return point_vector;
}

std::vector<arc_struct> dxf_data::return_arc_vector(){
	return arc_vector;
}

std::vector<text_struct> dxf_data::return_text_vector(){
	return text_vector;
}

std::vector<solid_struct> dxf_data::return_solid_vector(){
	return solid_vector;
}

std::vector<mtext_struct> dxf_data::return_mtext_vector(){
	return mtext_vector;
}

std::vector<text_attrib_struct> dxf_data::return_text_attrib_vector(){
	return text_attrib_vector;
}

std::vector<ellipse_struct> dxf_data::return_ellipse_vector(){
	return ellipse_vector;
}

std::vector<leader_struct> dxf_data::return_leader_vector(){
	return leader_vector;
}

std::vector<spline_struct> dxf_data::return_spline_vector(){
	return spline_vector;
}

std::vector<dimension_struct> dxf_data::return_dimension_vector(){
	return dimension_vector;
}

void dxf_data::clear_vector(){
	header_vector.clear();
	table_vector.clear();
	polyline_vertex_vector.clear();
	polyline_vector.clear();
	lwpolyline_vector.clear();
	circle_vector.clear();
	face3d_vector.clear();
	line_vector.clear();
 	point_vector.clear();
	arc_vector.clear();
	text_vector.clear();
	solid_vector.clear();
	mtext_vector.clear();
	text_attrib_vector.clear();
	ellipse_vector.clear();
	leader_vector.clear();
	spline_vector.clear();
	dimension_vector.clear();
}

dxf_data read_dxf_file(std::string in_filename, std::string out_filename)
{
    std::list<uint32_t> head_all;
    size_t name_len;
    char *ptr1, *ptr2;
    int code;
    int c;
    int i;

	dd.clear_vector();

    tol_sq = tol * tol;

    delta_angle = M_2PI / (double)segs_per_circle;
    sin_delta = sin(delta_angle);
    cos_delta = cos(delta_angle);
    /* get command line arguments */
    scale_factor = 1.0;
    // while ((c = bu_getopt(argc, argv, "cdvt:s:h?")) != -1) {
	// switch (c) {
	//     case 's':	/* scale factor */
	// 	scale_factor = atof(bu_optarg); //option? Maybe we can remove it? we don't need this driver
	// 	if (scale_factor < SQRT_SMALL_FASTF) {
	// 	    fprintf(out_test, "scale factor too small (%g < %g)\n", scale_factor, SQRT_SMALL_FASTF);
	// 	    //bu_exit(1, "%s", usage);
	// 		fprintf(out_test, "%s", usage);
	// 		exit(1);
	// 	}
	// 	break;
	//     case 'c':	/* ignore colors */
	// 	ignore_colors = 1;
	// 	break;
	//     case 'd':	/* debug */
	// 	//bu_debug = BU_DEBUG_COREDUMP;
	// 	break;
	//     case 't':	/* tolerance */
	// 	tol = atof(bu_optarg);
	// 	tol_sq = tol * tol;
	// 	break;
	//     case 'v':	/* verbose */
	// 	verbose = 1;
	// 	break;
	//     default:
	// 	//bu_exit(1, "%s", usage);
	// 	fprintf(out_test, "%s", usage);
	// 	exit(1);
	// }
    // }
	ignore_colors = 1;
	tol_sq = 0.01;
	verbose = 0;

    // if (argc - bu_optind < 2) {
	// //bu_exit(1, "%s", usage);
	// fprintf(out_test,"%s", usage );
	// exit(1);
    // }

    // dxf_file = argv[bu_optind++];
    // output_file = argv[bu_optind];
	dxf_file = strdup(in_filename.c_str());
    if ((dxf=fopen(dxf_file, "rb")) == NULL) {
	perror(dxf_file);
	// bu_exit(1, "Cannot open DXF file (%s)\n", dxf_file);
	// fprintf(stdout, "Cannot open DXF file (%s)\n", dxf_file);
	exit(1);
    }

    // if ((out_fp = fopen(output_file, "w")) == NULL) {//wdb_fopen(output_file)
	// perror(output_file);
	// //bu_exit(1, "Cannot open BRL-CAD geometry file (%s)\n", output_file);
	// fprintf(out_test, "Cannot open BRL-CAD geometry file (%s)\n", output_file);
	// exit(1);
    // }
	output_file = (char*)"output";
	// if((out_test=fopen(output_file, "w")) == NULL){
	// 	//fprintf(stdout, "Cannot open or create output file(%s) \n", output_file);
	// 	exit(1);
	// }
    ptr1 = strrchr(dxf_file, '/');
    if (ptr1 == NULL)
	ptr1 = dxf_file;
    else
	ptr1++;
    ptr2 = strchr(ptr1, '.');

    if (ptr2 == NULL)
	name_len = strlen(ptr1);
    else
	name_len = ptr2 - ptr1;

    base_name = (char *)calloc((unsigned int)name_len + 1, 1);
    strlcpy(base_name , ptr1 , name_len+1);

    // mk_id(out_fp, base_name);

    // BU_LIST_INIT(&block_head);
    // BU_LIST_INIT(&free_hd);
	std::list<block> block_list;
	std::list<state_data> state_stack;

    process_code[UNKNOWN_SECTION] = process_unknown_code;
    process_code[HEADER_SECTION] = process_header_code;
    process_code[CLASSES_SECTION] = process_classes_code;
    process_code[TABLES_SECTION] = process_tables_code;
    process_code[BLOCKS_SECTION] = process_blocks_code;
    process_code[ENTITIES_SECTION] = process_entity_code;
    process_code[OBJECTS_SECTION] = process_objects_code;
    process_code[THUMBNAILIMAGE_SECTION] = process_thumbnail_code;

    process_entities_code[UNKNOWN_ENTITY_STATE] = process_entities_unknown_code;
    process_entities_code[POLYLINE_ENTITY_STATE] = process_entities_polyline_code;
    process_entities_code[POLYLINE_VERTEX_ENTITY_STATE] = process_entities_polyline_vertex_code;
    process_entities_code[FACE3D_ENTITY_STATE] = process_3dface_entities_code;
    process_entities_code[LINE_ENTITY_STATE] = process_line_entities_code;
    process_entities_code[INSERT_ENTITY_STATE] = process_insert_entities_code;
    process_entities_code[POINT_ENTITY_STATE] = process_point_entities_code;
    process_entities_code[CIRCLE_ENTITY_STATE] = process_circle_entities_code;
    process_entities_code[ARC_ENTITY_STATE] = process_arc_entities_code;
    process_entities_code[DIMENSION_ENTITY_STATE] = process_dimension_entities_code;
    process_entities_code[TEXT_ENTITY_STATE] = process_text_attrib_entities_code;
    process_entities_code[SOLID_ENTITY_STATE] = process_solid_entities_code;
    process_entities_code[LWPOLYLINE_ENTITY_STATE] = process_lwpolyline_entities_code;
    process_entities_code[MTEXT_ENTITY_STATE] = process_mtext_entities_code;
    process_entities_code[ATTRIB_ENTITY_STATE] = process_text_attrib_entities_code;
    process_entities_code[ATTDEF_ENTITY_STATE] = process_text_attrib_entities_code;
    process_entities_code[ELLIPSE_ENTITY_STATE] = process_ellipse_entities_code;
    process_entities_code[LEADER_ENTITY_STATE] = process_leader_entities_code;
    process_entities_code[SPLINE_ENTITY_STATE] = process_spline_entities_code;

    process_tables_sub_code[UNKNOWN_TABLE_STATE] = process_tables_unknown_code;
    process_tables_sub_code[LAYER_TABLE_STATE] = process_tables_layer_code;

    /* create storage for circles */
    circle_pts = (double**)calloc(segs_per_circle, sizeof(double*));
    for (i = 0; i < segs_per_circle; i++) {
	circle_pts[i] = (double*)malloc(3*sizeof(double*));
	VSETALL(circle_pts[i], 0.0);
    }
    /* initialize state stack */
    //BU_LIST_INIT(&state_stack);

    /* create initial state */
    //BU_ALLOC(curr_state, struct state_data);
	//curr_state = (struct state_data *) malloc(sizeof(*curr_state));
	curr_state = new state_data();
    curr_state->file_offset = 0;
    curr_state->state = UNKNOWN_SECTION;
    curr_state->sub_state = UNKNOWN_ENTITY_STATE;
    MAT_IDN(curr_state->xform);

    /* make space for 5 layers to start */
    max_layers = 5;
    next_layer = 1;
    curr_layer = 0;
    //layers = (struct layer **)calloc(5, sizeof(struct layer));

    for (i = 0; i < max_layers; i++) {
		//layers[i] = (layer*)malloc(sizeof(layer[i]));
		layers.push_back(layer());
    }

    layers[0].name = strdup("noname");
    layers[0].color_number = 7;	/* default white */
    layers[0].vert_tree = create_vert_tree();
    //bu_ptbl_init(&layer[0].solids, 8, "layers[curr_layer].solids");

    curr_color = layers[0].color_number;
    curr_layer_name = strdup(layers[0].name); 

    while ((code=readcodes()) > -900) {
	process_code[curr_state->state](code);
	//fprintf(stdout, "current state(%d), code %d \n", curr_state->state, code);
    }
    //BU_LIST_INIT(&head_all);

    for (i = 0; i < next_layer; i++) {
	//struct bu_list head;
	std::list<uint32_t> head;
	size_t j;

	//BU_LIST_INIT(&head);

	if (layers[i].color_number < 0)
	    layers[i].color_number = 7;

	if(verbose)
	if (layers[i].curr_tri || layers[i].solids.size() || layers[i].m) {//BU_PTBL_LEN(&layers[i].solids) 
	    fprintf(out_test, "LAYER: %s, color = %d (%d %d %d)\n", layers[i].name, layers[i].color_number, V3ARGS(&rgb[layers[i].color_number*3]));
	}

	// if (layers[i].curr_tri && layers[i].vert_tree->curr_vert > 2) {
	//     sprintf(tmp_name, "bot.s%d", i);
	//     if (mk_bot(out_fp, tmp_name, RT_BOT_SURFACE, RT_BOT_UNORIENTED, 0,
	// 	       layers[i].vert_tree, layers[i].curr_tri, layers[i].vert_tree->the_array,
	// 	       layers[i].part_tris, (double *)NULL, (struct bu_bitv *)NULL)) {
	// 	fprintf(out_test, "Failed to make Bot\n");
	//     } else {
	// 	(void)mk_addmember(tmp_name, &head, NULL, WMOP_UNION);
	//     }
	// }

	/* obtain dynamic storage for a new wmember structure, we may ignore for std::list */
	// for (j = 0; j < BU_PTBL_LEN(&layers[i].solids); j++) {
	//     (void)mk_addmember((char *)BU_PTBL_GET(&layers[i].solids, j), &head, NULL, WMOP_UNION);
	//     free((char *)BU_PTBL_GET(&layers[i].solids, j)); 
	// }

	// if (layers[i].m) {
	//     char name[32];
	//     struct rt_sketch_internal *skt;

	//     sprintf(name, "sketch.%d", i);
	//     skt = nmg_wire_edges_to_sketch(layers[i].m);
	//     if (skt != NULL) {
	// 	mk_sketch(out_fp, name, skt);
	// 	(void) mk_addmember(name, &head, NULL, WMOP_UNION);
	// 	rt_curve_free(&skt->curve);
	// 	if (skt->verts)
	// 	    free(skt->verts); //"free verts");
	// 	free(skt);// "free sketch");
	//     }
	
	if(verbose){
		if (layers[i].line_count) {
			fprintf(out_test, "\t%zu lines\n", layers[i].line_count);
		}

		if (layers[i].solid_count) {
			fprintf(out_test, "\t%zu solids\n", layers[i].solid_count);
		}

		if (layers[i].polyline_count) {
			fprintf(out_test, "\t%zu polylines\n", layers[i].polyline_count);
		}

		if (layers[i].lwpolyline_count) {
			fprintf(out_test, "\t%zu lwpolylines\n", layers[i].lwpolyline_count);
		}

		if (layers[i].ellipse_count) {
			fprintf(out_test, "\t%zu ellipses\n", layers[i].ellipse_count);
		}

		if (layers[i].circle_count) {
			fprintf(out_test, "\t%zu circles\n", layers[i].circle_count);
		}

		if (layers[i].arc_count) {
			fprintf(out_test, "\t%zu arcs\n", layers[i].arc_count);
		}

		if (layers[i].text_count) {
			fprintf(out_test, "\t%zu texts\n", layers[i].text_count);
		}

		if (layers[i].mtext_count) {
			fprintf(out_test, "\t%zu mtexts\n", layers[i].mtext_count);
		}

		if (layers[i].attrib_count) {
			fprintf(out_test, "\t%zu attribs\n", layers[i].attrib_count);
		}

		if (layers[i].dimension_count) {
			fprintf(out_test, "\t%zu dimensions\n", layers[i].dimension_count);
		}

		if (layers[i].leader_count) {
			fprintf(out_test, "\t%zu leaders\n", layers[i].leader_count);
		}

		if (layers[i].face3d_count) {
			fprintf(out_test, "\t%zu 3d faces\n", layers[i].face3d_count);
		}

		if (layers[i].point_count) {
			fprintf(out_test, "\t%zu points\n", layers[i].point_count);
		}
		if (layers[i].spline_count) {
			fprintf(out_test, "\t%zu splines\n", layers[i].spline_count);
		}
	}

	}

	// if (!head.empty()) {//BU_LIST_NON_EMPTY(&head)
	//     unsigned char *tmp_rgb;
	//     char *comb_name = BU_VLS_INIT_ZERO;

	//     tmp_rgb = &rgb[layers[i].color_number*3];
	//     bu_vls_printf(&comb_name, "%s.c.%d", layers[i].name, i);
	//     if (mk_comb(out_fp, bu_vls_addr(&comb_name), &head, 1, NULL, NULL,
	// 		tmp_rgb, 1, 0, 1, 100, 0, 0, 0)) {
	// 	fprintf(out_test, "Failed to make region %s\n", layers[i].name);
	//     } else {
	// 	(void)mk_addmember(bu_vls_addr(&comb_name), &head_all, NULL, WMOP_UNION);
	//     }
	// }

    // }

    // if (!head_all.empty()) {//BU_LIST_NON_EMPTY(&head_all)
	// char *top_name = BU_VLS_INIT_ZERO; // variable length string??
	// int count = 0;

	// strcpy(top_name, "all");
	// while (db_lookup(out_fp->dbip, bu_vls_addr(&top_name), LOOKUP_QUIET) != RT_DIR_NULL) {
	//     count++;
	//     bu_vls_trunc(&top_name, 0);
	//     bu_vls_printf(&top_name, "all.%d", count);
	// }

	// (void)mk_comb(out_fp, bu_vls_addr(&top_name), &head_all, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, 0, 0);
    // }

	// static char* out_data_file;
	// out_data_file = (char*)"output_data";
	// if((out_data=fopen(out_data_file, "w")) == NULL){
	// 	//fprintf(stdout, "Cannot open or create output file(%s) \n", output_file);
	// 	exit(1);
	// }
    // for (i = 0; i < segs_per_circle; i++) {
	// 	fprintf(out_data, "Circle points (%d) : (%f, %f, %f) \n", i, circle_pts[i][X], circle_pts[i][Y], circle_pts[i][Z]);
    // }
	// if(polyline_vert_indices){
	// 	for( i = 0; i < polyline_vert_indices_count; i++){
	// 		fprintf(out_data, "polyline_vert_indices points (%d) : (%d, %d, %d) \n", i, polyline_vert_indices[X], polyline_vert_indices[Y], polyline_vert_indices[Z]);
	// 	}
	// }
	
	//fprintf(stdout, "layer: %d" ,layers[curr_layer].vert_tree->the_tree->vnode.coord);
    // return 0;
	delete curr_state;
	return dd;
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */