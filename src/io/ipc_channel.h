#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <ostream>
#include <string>

// Framing for the compute worker's payload channel (feature 32).
//
// Row 29 moved the worker's payloads from ASCII to binary but kept the file carrier; this
// removes the carrier. Payloads move over a data channel alongside the existing stdin/stdout
// line protocol, so the bytes never touch the filesystem and a preview stops creating one file
// per leaf PolySet.
//
// Why framing at all, rather than reusing the existing line protocol: payloads are arbitrary
// binary and routinely contain '\n' and '\0', and ComputeWorker::processOutput() drives off
// canReadLine(). A payload on that stream would be split at the first embedded newline. It also
// keeps the control channel responsive while a large payload is in flight, which is what stops
// the worker blocking on a full pipe while the GUI blocks on a control response.
//
// A message is named because a single preview sends several: products.json plus one payload per
// distinct leaf PolySet, and products.json refers to the leaves by path. Carrying the path the
// worker would have written lets the receiving side resolve those references from a map instead
// of the filesystem, leaving CsgInfo::read_products' logic intact.

// Ceilings exist so a corrupt length prefix -- the shape a worker killed mid-write leaves behind
// -- is rejected instead of being handed to reserve(). Both are far above anything real: the
// largest payload measured on this project is 191 MiB (`mim volume 2.scad`), and names are
// filesystem paths.
inline constexpr std::uint64_t kIpcMaxMessageSize = 16ull * 1024 * 1024 * 1024;
inline constexpr std::uint64_t kIpcMaxNameSize = 4096;

struct IpcMessage {
  std::string name;
  std::string payload;
};

std::string frame_ipc_message(const std::string& name, const std::string& payload);

// Canonical form of a payload name. Payload names are identifiers, never opened, and the two ends
// reach them by different routes that spell separators differently on Windows -- the geometry name
// arrives via fs::path::generic_string() with forward slashes, the metadata sidecars via plain
// concatenation of whatever the caller sent. Folding both to '/' here, and using this on the
// writing and the reading side alike, is what stops them drifting apart.
std::string ipc_payload_name(std::string name);

// Resolves a payload name to its bytes, or null if there is no such payload. The receiving side
// installs one so readers that used to open a path read from the channel's messages instead;
// an empty resolver means "read the filesystem", which is what every non-worker caller does.
using IpcPayloadResolver = std::function<const std::string *(const std::string&)>;

// Supplies a leaf that the receiving side already decoded as it arrived, so the decode is not
// repeated when the products are read. Null for a name it has not decoded.
class PolySet;
using IpcGeometryResolver = std::function<std::shared_ptr<const PolySet>(const std::string&)>;

// Worker side of the same idea. While a request is being served, the writers that would create
// the worker's output files hand their bytes here instead, keyed by the path they would have
// written. Naming messages after those paths is what lets products.json go on referring to its
// leaves by path with no change to how it is written or read.
//
// A bare namespace with process state, rather than an object threaded through the export
// machinery: reaching the writers means passing something through cmdline() and do_export(),
// whose signatures are shared with eight other feature branches. The worker is one request at a
// time in one process, so there is nothing here for a second caller to collide with.
namespace ipc_payload_sink {

// False everywhere except inside a compute worker serving a request, which is what keeps the
// ordinary CLI and GUI export paths writing real files.
bool collecting();
// Starts a request, discarding anything a previous failed one left behind. `out` is where payloads
// are written as they complete -- the worker's response stream (feature 34): a payload is sent the
// moment the next one is opened, rather than all of them being held until the request finishes, so
// the receiving side can begin work while the worker is still going.
void begin(std::ostream& out);
void end();
// A stream to write the named payload into.
//
// Opening a payload declares every previously opened one complete and sends it, so **the
// reference returned by an earlier open() must not be written to afterwards**. Every writer today
// opens a payload and fills it within the same expression or call, which is what makes this safe;
// a writer that interleaved two payloads would need this rethought.
std::ostream& open(const std::string& name);
// Not named emit(): Qt defines that as a macro expanding to nothing, and this header reaches
// Qt translation units through CsgInfo.h.
// Sends whatever has not been sent yet -- in practice the final payload, since the rest went out
// as they completed. Each is written as `payload\t<size>\n` followed by that many framed bytes;
// the count lets the reader switch out of line mode for exactly that many bytes, which is what
// allows payloads containing newlines to share the response stream.
void flush_pending();

}  // namespace ipc_payload_sink

// Accumulates fragments as they arrive off the channel and yields whole messages in order.
// A channel read has no relationship to a message boundary: an 8 MiB payload arrives in however
// many pieces the OS chooses, and several small messages can arrive in one.
class IpcMessageReader
{
public:
  void append(const char *data, std::size_t size);
  // Pops the oldest complete message. False means "not yet", not "never" -- unless failed().
  bool next(IpcMessage& message);
  // A framing violation. Unrecoverable: the stream position is no longer trustworthy.
  bool failed() const { return this->broken; }

private:
  std::string buffer;
  std::deque<IpcMessage> complete;
  bool broken = false;
};
