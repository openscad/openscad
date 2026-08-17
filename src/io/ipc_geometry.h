#pragma once

#include <iosfwd>
#include <memory>
#include <string>

class Geometry;
class PolySet;

// Suffix for the per-leaf payloads a preview writes. The writer in CsgInfo.cc and the cleanup
// loop in ComputeWorker::cleanupResult() must agree on it, or every preview leaks one file per
// distinct leaf PolySet -- 25 of them for a model like `extruder illustration.scad`.
inline constexpr auto kIpcGeometrySuffix = ".osig";

// Binary geometry transport between a window and its private compute worker (feature 32).
//
// Native byte order, native doubles, no versioned compatibility promise, and no attempt at
// portability: the writer and reader are always the same executable on the same machine, since
// the worker is launched from QCoreApplication::applicationFilePath(). That is what makes the
// raw layout safe, and it is also why nothing here pins byte order or struct layout -- a change
// to either lands in both ends at once. Do not reuse this format for anything a user can keep
// or move between machines; that is what the OFF/3MF exporters are for.
//
// Measured against the previous full-precision ASCII OFF transport, this is 44-94x faster
// per payload; the filesystem itself was never more than ~0.5% of the cost, so the encoding is
// where the speed came from. The file carrier it originally used has since been replaced by the
// response channel in ipc_channel.h, which is why the buffer form below is the one the worker
// transport uses; the path form remains for tests and for reading a payload off disk.
// Writer side matches the other exporters (ostream in, nothing returned) so it can be reached
// through the ordinary FileFormat dispatch; the reader matches import_off's shape.
void export_ipc_geometry(const std::shared_ptr<const Geometry>& geom, std::ostream& output);
void export_ipc_geometry(const PolySet& polyset, std::ostream& output);
std::unique_ptr<PolySet> import_ipc_geometry(const std::string& filename);
// The same decode from bytes already in memory. `name` only says which payload failed.
std::unique_ptr<PolySet> import_ipc_geometry_buffer(const char *data, std::size_t size,
                                                    const std::string& name);
