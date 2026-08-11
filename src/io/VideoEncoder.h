#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <ostream>
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
     Begins a stream. `out` must stay alive until close() returns.
     Returns false if the parameters are unusable (zero dimensions, zero fps).
   */
  virtual bool open(std::ostream& out, unsigned width, unsigned height, unsigned fps) = 0;

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
