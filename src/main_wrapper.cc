extern int openscad_main(int argc, char **argv);
extern int qInitResources_common();
extern int qInitResources_icons_chokusen();
extern int qInitResources_icons_chokusen_dark();
extern int qInitResources_mac();

int main(int argc, char **argv) {
#ifndef OPENSCAD_NOGUI
  (void)qInitResources_common();
  (void)qInitResources_icons_chokusen();
  (void)qInitResources_icons_chokusen_dark();
#endif
#ifdef __APPLE__
  (void)qInitResources_mac();
#endif
  return openscad_main(argc, argv);
}
