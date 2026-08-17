#include "io/ipc_channel.h"

// Stub: the framing tests in ipc_channel_test.cc are written first and must fail against this.
// Replaced by the real implementation once they are confirmed red for the right reason.

std::string frame_ipc_message(const std::string& /*name*/, const std::string& /*payload*/)
{
  return {};
}

void IpcMessageReader::append(const char * /*data*/, std::size_t /*size*/)
{
}

bool IpcMessageReader::next(IpcMessage& /*message*/)
{
  return false;
}
