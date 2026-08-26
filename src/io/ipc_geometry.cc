#include "io/ipc_geometry.h"

#include <cstdint>
#include <cstring>
#include <optional>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <vector>

#include "geometry/Geometry.h"
#include "geometry/PolySet.h"
#include "geometry/PolySetUtils.h"
#include "utils/printutils.h"

namespace fs = std::filesystem;

namespace {

// "OSIG": OpenSCAD internal geometry. Bumping kVersion is enough to invalidate a payload
// written by a different build, which is all the compatibility this format owes anyone --
// both ends are the same binary on the same machine.
constexpr uint32_t kMagic = 0x4749534f;
constexpr uint32_t kVersion = 2;

// A rendered model can be a list of separate bodies. The payload therefore always begins with
// this container, even for one body, so the reader never has to guess which shape it is holding.
struct ListHeader {
  uint32_t magic;
  uint32_t version;
  uint32_t bodyCount;
  uint32_t reserved;
};

struct Header {
  uint32_t magic;
  uint32_t version;
  uint32_t dimension;
  int32_t convexity;
  uint32_t flags;  // bit 0 triangular, bit 1 manifold
  uint32_t vertexCount;
  uint32_t polygonCount;
  uint32_t indexCount;  // total across all polygons
  uint32_t colorCount;
  uint32_t colorIndexCount;
};

template <typename T>
void append(std::vector<char>& out, const T& value)
{
  const auto offset = out.size();
  out.resize(offset + sizeof(T));
  std::memcpy(out.data() + offset, &value, sizeof(T));
}

// Reads from a bounds-checked cursor. Every read goes through this, so a truncated payload
// fails at the first short read instead of walking off the end of the buffer.
class Cursor
{
public:
  Cursor(const char *data, size_t size) : data(data), remaining(size) {}
  bool read(void *destination, size_t bytes)
  {
    if (bytes > remaining) return false;
    std::memcpy(destination, data, bytes);
    data += bytes;
    remaining -= bytes;
    return true;
  }
  template <typename T>
  bool read(T& value)
  {
    return read(&value, sizeof(T));
  }

private:
  const char *data;
  size_t remaining;
};

}  // namespace

namespace {

void appendPolySet(std::vector<char>& buffer, const PolySet& polyset)
{
  uint32_t indexCount = 0;
  for (const auto& face : polyset.indices) indexCount += face.size();

  Header header{
    kMagic,
    kVersion,
    polyset.getDimension(),
    static_cast<int32_t>(polyset.getConvexity()),
    static_cast<uint32_t>((polyset.isTriangular() ? 1u : 0u) | (polyset.isManifold() ? 2u : 0u)),
    static_cast<uint32_t>(polyset.vertices.size()),
    static_cast<uint32_t>(polyset.indices.size()),
    indexCount,
    static_cast<uint32_t>(polyset.colors.size()),
    static_cast<uint32_t>(polyset.color_indices.size())};

  buffer.reserve(buffer.size() + sizeof(Header) + polyset.vertices.size() * 3 * sizeof(double) +
                 (polyset.indices.size() + indexCount) * sizeof(uint32_t) +
                 polyset.colors.size() * 4 * sizeof(float) +
                 polyset.color_indices.size() * sizeof(int32_t));
  append(buffer, header);
  for (const auto& vertex : polyset.vertices) {
    const double xyz[3]{vertex.x(), vertex.y(), vertex.z()};
    const auto offset = buffer.size();
    buffer.resize(offset + sizeof(xyz));
    std::memcpy(buffer.data() + offset, xyz, sizeof(xyz));
  }
  // Length-prefixed per polygon: PolySet holds arbitrary n-gons, and a preview that assumed
  // triangles would silently drop every quad a real model produces.
  for (const auto& face : polyset.indices) {
    append(buffer, static_cast<uint32_t>(face.size()));
    for (const auto index : face) append(buffer, static_cast<int32_t>(index));
  }
  for (const auto& color : polyset.colors) {
    for (const auto channel : {color.r(), color.g(), color.b(), color.a()}) {
      append(buffer, static_cast<float>(channel));
    }
  }
  for (const auto index : polyset.color_indices) append(buffer, static_cast<int32_t>(index));
}

}  // namespace

void export_ipc_geometry(const std::shared_ptr<const Geometry>& geom, std::ostream& output)
{
  // Separate bodies are kept separate. Flattening them here is invisible in the preview but
  // destroys everything downstream that works per body, such as multi-file export.
  std::vector<std::shared_ptr<const Geometry>> bodies;
  if (const auto list = std::dynamic_pointer_cast<const GeometryList>(geom)) {
    for (const auto& child : list->flatten()) bodies.push_back(child.second);
  } else {
    bodies.push_back(geom);
  }

  std::vector<char> buffer;
  append(buffer, ListHeader{kMagic, kVersion, static_cast<uint32_t>(bodies.size()), 0});
  for (const auto& body : bodies) {
    // Same normalization the OFF exporter applies, so switching formats does not change which
    // geometry the GUI receives -- only how it is encoded.
    appendPolySet(buffer, *PolySetUtils::getGeometryAsPolySet(body));
  }
  output.write(buffer.data(), buffer.size());
}

void export_ipc_geometry(const PolySet& polyset, std::ostream& output)
{
  std::vector<char> buffer;
  append(buffer, ListHeader{kMagic, kVersion, 1, 0});
  appendPolySet(buffer, polyset);
  output.write(buffer.data(), buffer.size());
}

namespace {

std::optional<std::vector<char>> readWholeFile(const std::string& filename)
{
  std::error_code error;
  const auto size = fs::file_size(fs::u8path(filename), error);
  if (error) return {};
  std::vector<char> buffer(size);
  {
    std::ifstream stream(fs::u8path(filename), std::ios::binary);
    stream.read(buffer.data(), buffer.size());
    if (static_cast<size_t>(stream.gcount()) != buffer.size()) return {};
  }
  return buffer;
}

}  // namespace

std::shared_ptr<const Geometry> import_ipc_geometry(const std::string& filename)
{
  const auto buffer = readWholeFile(filename);
  if (!buffer) return {};
  return import_ipc_geometry_buffer(buffer->data(), buffer->size(), filename);
}

namespace {

bool readListHeader(Cursor& cursor, const std::string& name, ListHeader& listHeader)
{
  if (!cursor.read(listHeader)) return false;
  if (listHeader.magic != kMagic || listHeader.version != kVersion) {
    LOG(message_group::Error, "Compute worker geometry '%1$s' is not a usable payload.", name);
    return false;
  }
  return true;
}

std::unique_ptr<PolySet> readPolySet(Cursor& cursor)
{
  Header header{};
  if (!cursor.read(header)) return {};

  auto polyset = std::make_unique<PolySet>(header.dimension);
  polyset->setConvexity(header.convexity);
  polyset->setTriangular(header.flags & 1u);
  polyset->setManifold(header.flags & 2u);

  polyset->vertices.resize(header.vertexCount);
  for (auto& vertex : polyset->vertices) {
    double xyz[3];
    if (!cursor.read(xyz, sizeof(xyz))) return {};
    vertex = Vector3d(xyz[0], xyz[1], xyz[2]);
  }
  polyset->indices.resize(header.polygonCount);
  for (auto& face : polyset->indices) {
    uint32_t count = 0;
    if (!cursor.read(count)) return {};
    face.resize(count);
    for (auto& index : face) {
      int32_t value = 0;
      if (!cursor.read(value)) return {};
      index = value;
    }
  }
  polyset->colors.resize(header.colorCount);
  for (auto& color : polyset->colors) {
    float rgba[4];
    if (!cursor.read(rgba, sizeof(rgba))) return {};
    color = Color4f(rgba[0], rgba[1], rgba[2], rgba[3]);
  }
  polyset->color_indices.resize(header.colorIndexCount);
  for (auto& index : polyset->color_indices) {
    int32_t value = 0;
    if (!cursor.read(value)) return {};
    index = value;
  }
  return polyset;
}

}  // namespace

std::shared_ptr<const Geometry> import_ipc_geometry_buffer(const char *data, const std::size_t size,
                                                           const std::string& name)
{
  Cursor cursor(data, size);
  ListHeader listHeader{};
  if (!readListHeader(cursor, name, listHeader)) return {};

  Geometry::Geometries bodies;
  for (uint32_t i = 0; i < listHeader.bodyCount; ++i) {
    auto polyset = readPolySet(cursor);
    if (!polyset) return {};
    bodies.emplace_back(nullptr, std::shared_ptr<const Geometry>(std::move(polyset)));
  }

  // One body stays a bare PolySet: everything downstream reads that as "one body", and
  // wrapping it would change behaviour for every ordinary model.
  if (bodies.size() == 1) return bodies.front().second;
  return std::make_shared<GeometryList>(bodies);
}

std::unique_ptr<PolySet> import_ipc_polyset_buffer(const char *data, const std::size_t size,
                                                   const std::string& name)
{
  Cursor cursor(data, size);
  ListHeader listHeader{};
  if (!readListHeader(cursor, name, listHeader)) return {};
  if (listHeader.bodyCount != 1) {
    LOG(message_group::Error,
        "Compute worker geometry '%1$s' carries %2$d bodies where one was "
        "expected.",
        name, static_cast<int>(listHeader.bodyCount));
    return {};
  }
  return readPolySet(cursor);
}

std::unique_ptr<PolySet> import_ipc_polyset(const std::string& filename)
{
  const auto buffer = readWholeFile(filename);
  if (!buffer) return {};
  return import_ipc_polyset_buffer(buffer->data(), buffer->size(), filename);
}
