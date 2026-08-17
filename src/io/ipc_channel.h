#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
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
