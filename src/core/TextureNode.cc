/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "function.h"
#include "Arguments.h"
#include "Expression.h"
#include "Builtins.h"
#include "printutils.h"
#include "memory.h"
#include "UserModule.h"
#include "degree_trig.h"
#include "FreetypeRenderer.h"
#include "Parameters.h"
#include "io/import.h"
#include "io/fileutils.h"
#include "jpeglib.h"
#include "TextureNode.h"
#include "PlatformUtils.h"

struct my_error_mgr {
	struct jpeg_error_mgr pub;
	
	jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr *my_error_ptr;

std::vector<class TextureUV> textures;
GLuint textureIDs[TEXTURES_NUM];

TextureUV::TextureUV(std::string filepath, double uvscale)
{
	this->filepath = filepath;
	this->uvscale = uvscale;
}


static void my_error_exit (j_common_ptr cinfo)
{
	my_error_ptr myerr = (my_error_ptr) cinfo->err;
	
	longjmp(myerr->setjmp_buffer, 1);
}

struct jpeg_decompress_struct cinfo;
unsigned char *img_buf=NULL;

GLOBAL(int) read_JPEG_file (const char * filename)
{
  struct my_error_mgr jerr;
  FILE * infile;		/* source file */
  JSAMPARRAY buffer;		/* Output row buffer */
  int row_stride;		/* physical row width in output buffer */
  if ((infile = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    return 0;
  }
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return 0;
  }
  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, infile);
  (void) jpeg_read_header(&cinfo, TRUE);
  (void) jpeg_start_decompress(&cinfo);
  row_stride = cinfo.output_width * cinfo.output_components;
  img_buf=(unsigned char *) malloc(cinfo.output_width*cinfo.output_height*cinfo.output_components);
  /* Make a one-row-high sample array that will go away when done with image */
  buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

  while (cinfo.output_scanline < cinfo.output_height) {
    (void) jpeg_read_scanlines(&cinfo, buffer, 1);
    memcpy(img_buf+(cinfo.output_scanline-1)*row_stride, buffer[0],row_stride);
  }
  (void) jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  fclose(infile);
  return 1;
}

int loadTexture(unsigned char *textureptr, const char *path)
{
  int i,j;
  int xpos,ypos;
  // TODO cache feature
  if(!read_JPEG_file(path)) return 1;
  for(j=0;j<TEXTURE_SIZE;j++) {
    ypos=cinfo.output_height*j/TEXTURE_SIZE;
    for(i=0;i<TEXTURE_SIZE;i++)
    {
      xpos=cinfo.output_width*i/TEXTURE_SIZE;
      *(textureptr++) = img_buf[3*(cinfo.output_width*ypos+xpos)+0];
      *(textureptr++) = img_buf[3*(cinfo.output_width*ypos+xpos)+1];
      *(textureptr++) = img_buf[3*(cinfo.output_width*ypos+xpos)+2];
    }
  }
  if(img_buf != NULL) free(img_buf);
  return 0;
}


static std::shared_ptr<AbstractNode> builtin_texture(const ModuleInstantiation *inst, Arguments arguments, const Children& children)
{
  Location loc = Location::NONE;
  auto session = arguments.session();
  const Parameters parameters = Parameters::parse(std::move(arguments), loc, {}, {"file","uv" });
  std::string raw_filename = parameters.get("file", "");
  std::string file = lookup_file(raw_filename, loc.filePath().parent_path().string(), parameters.documentRoot());
  double uv=10;
  if (!parameters["uv"].isUndefined()) {
  	uv= parameters["uv"].toDouble();
 }

  TextureUV txt(file,uv); 
  textures.push_back(txt);
  return {};
}

void register_builtin_texture()
{
  Builtins::init("texture", new BuiltinModule(builtin_texture),
  {
    "texture(arg, ...)",
  });
	
}
