#include "import.h"
#include "PolySet.h"
#include "printutils.h"
#include "AST.h"

#include <fstream>
#include <boost/predef.h>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#if !defined(BOOST_ENDIAN_BIG_BYTE_AVAILABLE) && !defined(BOOST_ENDIAN_LITTLE_BYTE_AVAILABLE)
#error Byte order undefined or unknown. Currently only BOOST_ENDIAN_BIG_BYTE and BOOST_ENDIAN_LITTLE_BYTE are supported.
#endif

PolySet *import_obj(const std::string& filename, const Location& loc) {
  std::unique_ptr<PolySet> p = std::make_unique<PolySet>(3);
  FILE *in;
  char line[80];
  char *ptr, *ptr1, *ptr2;
  in=fopen(filename.c_str(),"r");
  if(in == NULL) return NULL;
  std::vector<Vector3d> pts;


  int lineno = 0;
  while (!feof(in)) {
    lineno++;
    ptr=fgets(line,sizeof(line),in);
    if(ptr == NULL) break;
    if(*ptr == '#') continue;
    ptr1=strchr(ptr,' ');
    if(ptr1 == NULL) continue; // No keyword separator
    *ptr1='\0';
    ptr1++;
    if(!strcmp(line,"v")) {
      Vector3d v;
      sscanf(ptr1,"%lf %lf %lf",&(v[0]),&(v[1]), &(v[2]));
      pts.push_back(v);
    } else if(!strcmp(line,"f")) {
      std::vector<int> inds;
      int end=0;
      int ind;
      p->append_poly();
      while(!end){
        while(*ptr1 ==  ' ' || *ptr1 == '\t') ptr1++;
          ptr2=ptr1;
        while(*ptr2 !=  ' ' && *ptr2 != '\t' && *ptr2 != '\0' ) ptr2++;
          if(*ptr2 == '\0') end=1;
        *ptr2='\0';
        sscanf(ptr1,"%d",&ind);
        if(ind >= 1 && ind  <= pts.size())
          p->append_vertex(pts[ind-1][0], pts[ind-1][1], pts[ind-1][2]);
        else
          LOG(message_group::Warning, Location::NONE, "", "Index %1$d out of range in Line %2$d", filename, lineno);
            
        ptr1=ptr2+1;
            
      }
    } else if(!strcmp(line,"vt")) { // ignore texture coords
    } else if(!strcmp(line,"vn")) { // ignore normal coords
    } else if(!strcmp(line,"mtllib")) { // ignore material lib
    } else if(!strcmp(line,"usemtl")) { // ignore use mtl
    } else if(!strcmp(line,"o")) { // ignore object name
    } else if(!strcmp(line,"s")) { // ignore smoothening
    } else if(!strcmp(line,"g")) { // ignore group name
    } else {
      printf("Unknown Line %s\n",line);
    }
  }
  fclose(in);
  return p.release();
}
