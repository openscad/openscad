#include <GL/glew.h>
#include <stdio.h>
#include <setjmp.h>
#include <jerror.h>
#include <stdlib.h>
#include <string.h>

#define TEXTURE_SIZE	512
#define TEXTURES_NUM	7

class TextureUV
{
	public:
		TextureUV(std::string filepath, double uv);
		std::string filepath;
		float uvscale;
};

extern GLuint textureIDs[TEXTURES_NUM];
extern std::vector<TextureUV> textures;
int loadTexture(unsigned char *textptr, const char *path);
