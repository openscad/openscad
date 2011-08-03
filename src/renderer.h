#ifndef RENDERER_H_
#define RENDERER_H_

class Renderer
{
public:
	virtual ~Renderer() {}
	virtual void draw(bool showfaces, bool showedges) const = 0;
};

#endif // RENDERER_H
