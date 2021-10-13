/*
 *  Reported by Justin Charette
 *  Causes a crash in CSGTermNormalizer::normalize() when CSG element count
 *  exceeds limit setting in preferences (verified with default value of 2000).
 */


$fn=20;

/* donut (r1, r2, t) {{{
  r1 = radius of torus
  r2 = radius of torus cross section (circle)
  t  = thickness of shell (t == 0 is 
*/
module donut (r1, r2, t=0) {
  difference() {
    rotate_extrude( convexity=6 ) {
      translate([r1, 0, 0]) {
        circle( r = r2 );
      }
    }
    // (t == 0 ? solid : hollow ) 
    if (t > 0) {
      rotate_extrude( convexity=6 ) {
        translate([r1, 0, 0]) {
          circle( r = r2-t );
        }
      }
    }
  }
} //}}}

/* half donut (r1, r2, t, round) {{{
  r1     = radius of torus
  r2     = radius of torus cross section (circle)
  t      = thickness of shell
  round  = trim ends of semi-torus so they are round
*/
module half_donut (r1, r2, t=1, round=false) {
  difference() {
    donut( r1, r2, t );
    difference() {
      translate( [0, -((r1+r2)/2+0.5), 0] )
        scale( [2*(r1+r2)+1, r1+r2+1, 2*r2+1] )
          square( 1, center=true );
      if (round) {
        rotate( 90, [0, 1, 0] )
          cylinder( 2*(r1+r2)+2, r2, r2, center=true );
      }
    }
  }
} //}}}

/* donut flange (r1, r2, a1, a2, a_step, t, round) {{{
  r1      = radius of torus
  r2      = radius of torus cross section (circle)
  a1      = starting angle of flange rotation
  a2      = stopping angle of flange rotation
  a_step  = increment size of flange rotation
  t       = thickness of shell (t == 0 is solid, t in (0, r2) is hollow)
  round   = (true/false) to trim ends of semi-torus so they are round
*/
module donut_flange (r1, r2, a1, a2, a_step=1, t=0, round=false) {
  difference() {
    union() {
      for (a = [a1:a_step:a2]) {
        rotate( a, [1, 0, 0] )
          half_donut( r1, r2, round );
      }
    }
    // (t == 0 ? solid : hollow ) 
    if (t > 0) {
      union() {
        for (a = [a1:a_step:a2]) {
          rotate( a, [1, 0, 0] )
            half_donut( r1, r2-t, round );
        }
      }
    }
  }
} //}}}

donut( 20, 5 );
donut_flange( 20, 5, 0, 50, 10, t=1, round=false );

// vim: set et sw=2 ts=2 sts=2 ai sta sr fdm=marker:
