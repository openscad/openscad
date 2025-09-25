#pragma once

class Surface
{
public:
  Vector3d refpt, normdir;
  virtual void display(const std::vector<Vector3d>& vertices);
  virtual void reverse(void);
  virtual int operator==(const Surface& other);
  virtual int pointMember(std::vector<Vector3d>& vertices, Vector3d pt);
};

class CylinderSurface : public Surface
{
public:
  CylinderSurface(Vector3d center, Vector3d normdir, double r);
  void display(const std::vector<Vector3d>& vertices);
  void reverse(void);
  int operator==(const CylinderSurface& other);
  virtual int pointMember(std::vector<Vector3d>& vertices, Vector3d pt);

  double r;

private:
  virtual int operator==(const Surface& other) { return 0; }
};
