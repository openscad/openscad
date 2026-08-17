#include "io/ipc_channel.h"

#include <cstring>
#include <utility>

// Frame layout: [uint64 nameSize][name][uint64 payloadSize][payload].
//
// Native byte order and native width, for the same reason ipc_geometry.h gives: both ends are
// the same executable on the same machine, launched from QCoreApplication::applicationFilePath().
// Nothing here is written to anywhere a user can keep it.

namespace {

void appendSize(std::string& out, std::uint64_t value)
{
  out.append(reinterpret_cast<const char *>(&value), sizeof(value));
}

// Reads a length field at `offset`, or reports that the buffer does not hold one yet. `offset`
// is never past the end, so the subtraction cannot wrap.
bool readSize(const std::string& buffer, std::size_t offset, std::uint64_t& value)
{
  if (buffer.size() - offset < sizeof(value)) return false;
  std::memcpy(&value, buffer.data() + offset, sizeof(value));
  return true;
}

}  // namespace

std::string frame_ipc_message(const std::string& name, const std::string& payload)
{
  std::string framed;
  framed.reserve(2 * sizeof(std::uint64_t) + name.size() + payload.size());
  appendSize(framed, name.size());
  framed += name;
  appendSize(framed, payload.size());
  framed += payload;
  return framed;
}

void IpcMessageReader::append(const char *data, std::size_t size)
{
  if (this->broken) return;
  this->buffer.append(data, size);

  // Consume as many whole messages as the buffer now holds. Every `break` below means "not
  // enough bytes yet" and leaves the buffer untouched from `offset` on, so the next append
  // resumes from the same place. The only other exit is a length that cannot be legitimate,
  // which is unrecoverable: a bad length means the stream position is no longer known, so
  // nothing after it can be trusted either.
  std::size_t offset = 0;
  while (true) {
    std::uint64_t nameSize = 0;
    if (!readSize(this->buffer, offset, nameSize)) break;
    if (nameSize > kIpcMaxNameSize) {
      this->broken = true;
      break;
    }
    auto cursor = offset + sizeof(nameSize);
    if (this->buffer.size() - cursor < nameSize) break;
    // Bounded by kIpcMaxNameSize above, so the narrowing is safe.
    const auto nameLength = static_cast<std::size_t>(nameSize);
    auto name = this->buffer.substr(cursor, nameLength);
    cursor += nameLength;

    std::uint64_t payloadSize = 0;
    if (!readSize(this->buffer, cursor, payloadSize)) break;
    if (payloadSize > kIpcMaxMessageSize) {
      this->broken = true;
      break;
    }
    cursor += sizeof(payloadSize);
    if (this->buffer.size() - cursor < payloadSize) break;
    // The check above proves the payload is already in the buffer, so it fits in a size_t.
    const auto payloadLength = static_cast<std::size_t>(payloadSize);

    this->complete.push_back({std::move(name), this->buffer.substr(cursor, payloadLength)});
    offset = cursor + payloadLength;
  }

  // Note this is a no-op while a large payload is still arriving -- offset stays 0 until a
  // message completes -- so accumulating one in many small reads does not repeatedly shuffle it.
  this->buffer.erase(0, offset);
}

bool IpcMessageReader::next(IpcMessage& message)
{
  // Messages parsed before the framing broke are deliberately not handed out: if a length was
  // wrong, there is no way to know the earlier boundaries were right either.
  if (this->broken || this->complete.empty()) return false;
  message = std::move(this->complete.front());
  this->complete.pop_front();
  return true;
}
