#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

/*!
   Encodes a sequence of same-sized RGBA frames into an animation container.

   Deliberately free of Qt: the same encoders serve the GUI's animation dump and the
   headless `--animate` CLI path, and the unit tests build in HEADLESS configurations
   where no Qt GUI is available. Callers holding a QImage convert once with
   QImage::convertToFormat(QImage::Format_RGBA8888) and pass bits()/bytesPerLine().
 */
class VideoEncoder
{
public:
  virtual ~VideoEncoder() = default;

  VideoEncoder() = default;
  VideoEncoder(const VideoEncoder&) = delete;
  VideoEncoder& operator=(const VideoEncoder&) = delete;

  /*!
     Begins writing `path`, truncating any existing file. Returns false if the file
     cannot be opened or the parameters are unusable (zero dimensions, zero fps).

     A path rather than a std::ostream because two of the four backends cannot write
     to a stream at all: the vendored gif.h writes through a FILE*, and Qt's
     QMediaRecorder takes an output URL.
   */
  virtual bool open(const std::string& path, unsigned width, unsigned height, unsigned fps) = 0;

  /*!
     Appends one frame. `rgba` points at `height * stride` bytes, 4 bytes per pixel,
     top row first. Must be called between open() and close().
   */
  virtual bool addFrame(const uint8_t *rgba, std::size_t stride) = 0;

  //! Finalizes the stream. No further frames may be added.
  virtual bool close() = 0;

  /*!
     Returns an encoder for a lowercase file suffix without the dot ("gif", "apng",
     "avi"), or nullptr if the suffix is not one we encode ourselves. Container
     formats handed to the platform encoder (mp4, mov, ...) are not created here —
     that backend needs Qt and lives with the GUI.
   */
  static std::unique_ptr<VideoEncoder> create(const std::string& suffix);
};

/*!
   The final path component's suffix, lowercased and without the dot; empty when there
   is none. A dot in a directory name is not a suffix.
 */
std::string outputSuffix(const std::string& path);

/*!
   Output path for one frame of a numbered still-image sequence: "spin.png" with frame
   7 becomes "spin00007.png". Shared by the GUI's frame dump and the CLI's --animate so
   the two cannot drift apart.
 */
std::string numberedFramePath(const std::string& path, unsigned frame);
