#pragma once

#include <memory>
#include <string>

class PolySet;

// Binary geometry transport between a window and its private compute worker (feature 32).
//
// Native byte order, native doubles, no versioned compatibility promise: the two ends are
// always the same binary on the same machine. Do not reuse this for anything a user can keep
// or move between machines -- that is what the OFF/3MF exporters are for.
//
// Measured against the previous full-precision ASCII OFF transport, this is 44-94x faster
// per payload; the filesystem itself was never more than ~0.5% of the cost, which is why the
// file carrier is retained and only the encoding changed.
bool write_ipc_geometry(const PolySet& polyset, const std::string& filename);
std::unique_ptr<PolySet> read_ipc_geometry(const std::string& filename);
