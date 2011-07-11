#ifndef BUILTIN_H_
#define BUILTIN_H_

#include <QHash>

extern QHash<QString, class AbstractFunction*> builtin_functions;
extern void initialize_builtin_functions();
extern void destroy_builtin_functions();

extern QHash<QString, class AbstractModule*> builtin_modules;
extern void initialize_builtin_modules();
extern void destroy_builtin_modules();

extern void register_builtin_csgops();
extern void register_builtin_transform();
extern void register_builtin_primitives();
extern void register_builtin_surface();
extern void register_builtin_control();
extern void register_builtin_render();
extern void register_builtin_import();
extern void register_builtin_projection();
extern void register_builtin_cgaladv();
extern void register_builtin_dxf_linear_extrude();
extern void register_builtin_dxf_rotate_extrude();
extern void register_builtin_viewport();
extern void initialize_builtin_dxf_dim();

#endif
