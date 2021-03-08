// color scheme generator
// use customizer to select colors
// switch between perspective and 3d anaglyph to see what the colors look like
// copy and paste console output to /usr/share/openscad/color-schemes/render/redcyanglasses.json

// customizer variables

light_hue = 75; // [45:105]
dark_hue = 285; // [255:315]
saturation = 95; // [0:100]
dark_scheme = false;

module __end_of_customizer_variables__ () {}  // marks end of customizer variables

hue_front = dark_scheme ? dark_hue : light_hue;
hue_back = dark_scheme ? light_hue : dark_hue;
lightness_background = dark_scheme ? 0 : 100;

// color scheme.
// parameters to hsv_deg are hue [0:360], saturation [0:100] and value [0:100]

background         = hsv_deg(0, 0, lightness_background);
highlight          = hsv_deg(hue_back,saturation,100);
axes_color         = hsv_deg(hue_back,saturation,100);
opencsg_face_front = hsv_deg(hue_front,saturation,100);
opencsg_face_back  = hsv_deg(hue_back,saturation,100);
cgal_face_front    = hsv_deg(hue_front,saturation,100);
cgal_face_back     = hsv_deg(hue_back,saturation,100);
cgal_face_2d       = hsv_deg(hue_front,saturation,100);
cgal_edge_front    = hsv_deg(hue_back,saturation,100);
cgal_edge_back     = hsv_deg(hue_back,saturation,100);
cgal_edge_2d       = hsv_deg(hue_back,saturation,100);
crosshair          = hsv_deg(hue_back,saturation,100);

// set viewport
$vpt=[68, 88, 5];
$vpr=[0, 0, 0];
$vpd=550;

function hsv_deg(h, s, v) = hsv(h/360,s/100,v/100, 1);
function hsv(h, s = 1, v = 1, a = 1, p, q, t) = (p == undef || q == undef || t == undef) // by LightAtPlay
	? hsv(
		(h%1) * 6,
		s<0?0:s>1?1:s,
		v<0?0:v>1?1:v,
		a,
		(v<0?0:v>1?1:v) * (1 - (s<0?0:s>1?1:s)),
		(v<0?0:v>1?1:v) * (1 - (s<0?0:s>1?1:s) * ((h%1)*6-floor((h%1)*6))),
		(v<0?0:v>1?1:v) * (1 - (s<0?0:s>1?1:s) * (1 - ((h%1)*6-floor((h%1)*6))))
	)
	:
	h < 1 ? [v,t,p,a] :
	h < 2 ? [q,v,p,a] :
	h < 3 ? [p,v,t,a] :
	h < 4 ? [p,q,v,a] :
	h < 5 ? [t,p,v,a] :
	        [v,p,q,a];

hexchars="0123456789ABCDEF";
function hex(a) = str(hexchars[(a / 16) % 16] , hexchars[a % 16]);
function color_to_hex(c) = str(hex(c[0]*255), hex(c[1]*255), hex(c[2]*255));

cs = str(
    "copy and paste to redcyanglasses.json:\n",
    "{\n",
    "    \"name\" : \"3D Glasses\",\n",
    "    \"index\" : 2001,\n",
    "    \"show-in-gui\" : true,\n",
    "    \"description\" : \"3d anaglyph\",\n",
    "    \"_comment\" : \"created by colorscheme.scad, ", dark_scheme ? "dark" : "light", " scheme, light hue ", str(light_hue), ", dark hue ", str(dark_hue), ", saturation ", str(saturation), "\",\n",
    "\n",
    "    \"colors\" : {\n",
    "        \"background\" :         \"#", color_to_hex(background), "\",\n",
    "        \"highlight\" :          \"#", color_to_hex(highlight), "80\",\n",
    "        \"axes-color\" :         \"#", color_to_hex(axes_color), "\",\n",
    "        \"opencsg-face-front\" : \"#", color_to_hex(opencsg_face_front), "\",\n",
    "        \"opencsg-face-back\" :  \"#", color_to_hex(opencsg_face_back), "\",\n",
    "        \"cgal-face-front\" :    \"#", color_to_hex(cgal_face_front), "\",\n",
    "        \"cgal-face-back\" :     \"#", color_to_hex(cgal_face_back), "\",\n",
    "        \"cgal-face-2d\" :       \"#", color_to_hex(cgal_face_2d), "\",\n",
    "        \"cgal-edge-front\" :    \"#", color_to_hex(cgal_edge_front), "\",\n",
    "        \"cgal-edge-back\" :     \"#", color_to_hex(cgal_edge_back), "\",\n",
    "        \"cgal-edge-2d\" :       \"#", color_to_hex(cgal_edge_2d), "\",\n",
    "        \"crosshair\" :          \"#", color_to_hex(crosshair), "\"\n",
    "    }\n",
    "}\n"
    );
    
echo(cs);    

// display colors

colors = [
    background,
    highlight,
    axes_color,
    opencsg_face_front,
    opencsg_face_back,
    cgal_face_front,
    cgal_face_back,
    cgal_face_2d,
    cgal_edge_front,
    cgal_edge_back,
    cgal_edge_2d,
    crosshair];
    
names = [
    "background",
    "highlight",
    "axes_color",
    "opencsg_face_front",
    "opencsg_face_back",
    "cgal_face_front",
    "cgal_face_back",
    "cgal_face_2d",
    "cgal_edge_front",
    "cgal_edge_back",
    "cgal_edge_2d",
    "crosshair"];
  
for (i = [0:len(colors)-1]) {
    translate([0, i*15, 0])
    color(colors[i])
    cube(10);
    translate([15, i*15, 0])
    text(names[i], valign = "bottom");
}

// not truncated
