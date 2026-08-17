// Framing tests for the compute worker's payload channel (feature 32).
//
// These pin the carrier, not the payload format: what a message is made of is
// ipc_geometry's business and is tested in ipc_geometry_test.cc. What matters here is that
// bytes handed to the channel come back out identical, whole, in order, and that a truncated
// or corrupt frame is refused rather than half-delivered.
//
// The delivery-fragmentation cases are the point. A file is present and complete or absent;
// a channel hands over whatever arrived, so every off-by-one in the framing shows up as a
// message that is silently short. Feeding the same bytes one at a time is the cheapest way to
// catch that, and it is why these tests exist before any transport code.
#include "io/ipc_channel.h"

#include <catch2/catch_all.hpp>

#include <cstdint>
#include <string>

namespace {

// Deliberately full of the bytes that break a line protocol: embedded newlines, NULs, a
// trailing newline, and high bytes. This is what a real binary payload looks like, and
// reproducing it here is what makes the "why not just use stdout" answer a test rather than
// a comment.
std::string hostilePayload()
{
  std::string bytes = "OSIG";
  bytes += '\0';
  bytes += "\n\r\n";
  bytes += '\0';
  bytes += "\x80\xff not a line\n";
  bytes += std::string(1000, '\n');
  return bytes;
}

}  // namespace

TEST_CASE("ipc channel round-trips a payload delivered in one chunk", "[ipc-channel]")
{
  const auto payload = hostilePayload();
  const auto framed = frame_ipc_message("/tmp/openscad-worker-abc/result.osig", payload);

  IpcMessageReader reader;
  reader.append(framed.data(), framed.size());

  IpcMessage message;
  REQUIRE(reader.next(message));
  CHECK(message.name == "/tmp/openscad-worker-abc/result.osig");
  CHECK(message.payload == payload);
  CHECK_FALSE(reader.failed());
  // Exactly one message, and nothing left over.
  CHECK_FALSE(reader.next(message));
}

TEST_CASE("ipc channel reassembles a payload delivered one byte at a time", "[ipc-channel]")
{
  const auto payload = hostilePayload();
  const auto framed = frame_ipc_message("leaf-7.osig", payload);

  IpcMessageReader reader;
  IpcMessage message;
  // Every byte but the last must leave the message incomplete. A reader that yields early
  // here is one that would hand a half mesh to the viewport.
  for (std::size_t index = 0; index + 1 < framed.size(); ++index) {
    reader.append(framed.data() + index, 1);
    REQUIRE_FALSE(reader.next(message));
    REQUIRE_FALSE(reader.failed());
  }
  reader.append(framed.data() + framed.size() - 1, 1);

  REQUIRE(reader.next(message));
  CHECK(message.name == "leaf-7.osig");
  CHECK(message.payload == payload);
}

TEST_CASE("ipc channel delivers several messages from one chunk in order", "[ipc-channel]")
{
  // The F5 shape: products.json plus one payload per distinct leaf PolySet, which for a model
  // like `extruder illustration.scad` is 25 of them arriving back to back.
  std::string stream;
  stream += frame_ipc_message("products.json", "{\"products\":[]}");
  stream += frame_ipc_message("leaf-0.osig", hostilePayload());
  stream += frame_ipc_message("leaf-1.osig", "");

  IpcMessageReader reader;
  reader.append(stream.data(), stream.size());

  IpcMessage message;
  REQUIRE(reader.next(message));
  CHECK(message.name == "products.json");
  CHECK(message.payload == "{\"products\":[]}");

  REQUIRE(reader.next(message));
  CHECK(message.name == "leaf-0.osig");
  CHECK(message.payload == hostilePayload());

  // An empty payload is a real case -- a leaf can render to nothing -- and must be
  // distinguishable from "no message yet".
  REQUIRE(reader.next(message));
  CHECK(message.name == "leaf-1.osig");
  CHECK(message.payload.empty());

  CHECK_FALSE(reader.next(message));
  CHECK_FALSE(reader.failed());
}

TEST_CASE("ipc channel withholds a truncated message rather than delivering it short", "[ipc-channel]")
{
  // What a worker killed mid-write leaves on the channel.
  const auto framed = frame_ipc_message("result.osig", hostilePayload());

  IpcMessageReader reader;
  reader.append(framed.data(), framed.size() - 1);

  IpcMessage message;
  CHECK_FALSE(reader.next(message));
  // Not a framing violation -- the reader cannot know the writer died rather than paused.
  // Detecting end-of-stream is the caller's job; not inventing a message is this one's.
  CHECK_FALSE(reader.failed());
}

TEST_CASE("ipc channel rejects an implausible length prefix instead of allocating", "[ipc-channel]")
{
  auto framed = frame_ipc_message("result.osig", "small");

  SECTION("payload length")
  {
    // Overwrite the payload length with something a corrupt frame would carry. Located by
    // searching rather than by offset so the test does not pin the header layout.
    const auto position = framed.rfind(std::string("small"));
    REQUIRE(position != std::string::npos);
    const std::uint64_t absurd = UINT64_MAX;
    REQUIRE(position >= sizeof(absurd));
    framed.replace(position - sizeof(absurd), sizeof(absurd),
                   std::string(reinterpret_cast<const char *>(&absurd), sizeof(absurd)));
  }

  SECTION("name length")
  {
    const std::uint64_t absurd = kIpcMaxNameSize + 1;
    framed.replace(0, sizeof(absurd),
                   std::string(reinterpret_cast<const char *>(&absurd), sizeof(absurd)));
  }

  IpcMessageReader reader;
  reader.append(framed.data(), framed.size());

  IpcMessage message;
  CHECK_FALSE(reader.next(message));
  CHECK(reader.failed());
}
