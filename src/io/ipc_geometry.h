#pragma once

#include <iosfwd>
#include <memory>
#include <string>

class Geometry;
class PolySet;

// Suffix for the per-leaf payloads a preview writes. The writer in CsgInfo.cc and the cleanup
// loop in ComputeWorker::cleanupResult() must agree on it, or every preview leaks one file per
// leaf -- 428 of them for a model like `extruder illustration.scad`.
inline constexpr auto kIpcGeometrySuffix = ".osig";

// Binary geometry transport between a window and its private compute worker (feature 32).
//
// Native byte order, native doubles, no versioned compatibility promise: the two ends are
// always the same binary on the same machine. Do not reuse this for anything a user can keep
// or move between machines -- that is what the OFF/3MF exporters are for.
//
// Measured against the previous full-precision ASCII OFF transport, this is 44-94x faster
// per payload; the filesystem itself was never more than ~0.5% of the cost, which is why the
// file carrier is retained and only the encoding changed.
// Writer side matches the other exporters (ostream in, nothing returned) so it can be reached
// through the ordinary FileFormat dispatch; the reader matches import_off's shape.
void export_ipc_geometry(const std::shared_ptr<const Geometry>& geom, std::ostream& output);
void export_ipc_geometry(const PolySet& polyset, std::ostream& output);
std::unique_ptr<PolySet> import_ipc_geometry(const std::string& filename);
