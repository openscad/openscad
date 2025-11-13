
struct lineS {
  Vector2d p1;
  Vector2d p2;
  int dashed = 0;
};

struct labelS {
  Vector2d pt;
  double rot;
  double size;
  char text[10];
};

struct sheetS {
  Vector2d min, max;
  std::vector<lineS> lines;
  std::vector<labelS> label;
};

struct plotSettingsS {
  double lasche;
  double rand;
  const char *paperformat;
  double paperwidth, paperheight;
  int bestend;
};

struct plateS {
  std::vector<Vector2d> pt;     // actual points
  std::vector<Vector2d> pt_l1;  // leap starts
  std::vector<Vector2d> pt_l2;  // leap ends
  std::vector<Vector2d> bnd;    // Complete boundary representing points and leps
  int done;
};

struct connS {
  unsigned int p1, f1, p2, f2;
  int done;
};

std::vector<sheetS> fold_3d(std::shared_ptr<const PolySet> ps, const plotSettingsS& plot_s);
