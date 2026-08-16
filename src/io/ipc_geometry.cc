#include "io/ipc_geometry.h"

// ponytail: deliberately not implemented yet -- red test first, per the project's
// tests-first rule. The real encoding lands in the next commit.
bool write_ipc_geometry(const PolySet&, const std::string&) { return false; }

std::unique_ptr<PolySet> read_ipc_geometry(const std::string&) { return {}; }
