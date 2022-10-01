#version 110

uniform vec4 color1, color2;
varying vec3 vBC;
varying vec4 shading;

 // float divlist[8]=float[8](1.,5.,10.,50,100.,500.,1000.,5000.,10000.);
 float divlist  =1.;


void main(void) {

	vec3  col=color1.xyz;
	float linehardness;	
	float modx, mody, modz;
	float linew=0.025 ; 
	float divisions=1.;
	
		 for(int i=0;i<4;i++){
			 for(int j=1;j<=6;j+=4){
		 divisions= (pow(10.,float(i))*float (j));
	modx=abs(mod(shading.x+divisions*.5,divisions)-divisions*.5);
	mody=abs(mod(shading.y+divisions*.5,divisions)-divisions*.5);
	modz=abs(mod(shading.z+divisions*.5,divisions)-divisions*.5);
	linehardness=modx+mody+modz;	
	
	 if( modx<=linew || mody<=linew || modz<=linew ) 
		 {
		 
 		col =  mix(col,col-vec3(.025), smoothstep(0.,linew*1.5,linehardness));
		
		 }
		 
linew*=1.25;
		 }}
		
 
		
		
   gl_FragColor =  vec4  (col , color1.a );
}
