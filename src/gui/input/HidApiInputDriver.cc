/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 *  Initial implementation by Jochen Kunz and Gert Menke provided as
 *  Public Domain.
 */

#include "gui/input/HidApiInputDriver.h"

#include <ios>
#include <sstream>
#include <cstdint>
#include <bitset>
#include <boost/format.hpp>
#include <chrono>
#include <cmath>
#include <codecvt>
#include <fstream>
#include <iomanip>
#include <string>

#include "gui/Settings.h"
#include "platform/PlatformUtils.h"
#include "gui/input/InputDriverEvent.h"
#include "gui/input/InputDriverManager.h"
#include "utils/printutils.h"

static constexpr int BUFLEN = 64;
static constexpr int MAX_LOG_SIZE = 20 * 1024;

namespace ch = std::chrono;

static std::ofstream logstream;
static ch::system_clock::time_point logtime;

// http://www.linux-usb.org/usb.ids
// http://www.3dconnexion.eu/nc/service/faq/show_faq/7ece50ed-0b39-b57e-d3b2-4afd9420604e.html
static const struct device_id device_ids[] = {
  { 0x046d, 0xc603, &HidApiInputDriver::hidapi_decode_axis, &HidApiInputDriver::hidapi_decode_button, "3Dconnexion Spacemouse Plus XT"},
  { 0x046d, 0xc605, &HidApiInputDriver::hidapi_decode_axis, &HidApiInputDriver::hidapi_decode_button, "3Dconnexion CADman"},
  { 0x046d, 0xc606, &HidApiInputDriver::hidapi_decode_axis, &HidApiInputDriver::hidapi_decode_button, "3Dconnexion Spacemouse Classic"},
  { 0x046d, 0xc621, &HidApiInputDriver::hidapi_decode_axis, &HidApiInputDriver::hidapi_decode_button, "3Dconnexion Spaceball 5000"},
  { 0x046d, 0xc623, &HidApiInputDriver::hidapi_decode_axis, &HidApiInputDriver::hidapi_decode_button, "3Dconnexion Space Traveller 3D Mouse"},
  { 0x046d, 0xc625, &HidApiInputDriver::hidapi_decode_axis, &HidApiInputDriver::hidapi_decode_button, "3Dconnexion Space Pilot 3D Mouse"},
  { 0x046d, 0xc626, &HidApiInputDriver::hidapi_decode_axis, &HidApiInputDriver::hidapi_decode_button, "3Dconnexion Space Navigator 3D Mouse"},
  { 0x046d, 0xc627, &HidApiInputDriver::hidapi_decode_axis, &HidApiInputDriver::hidapi_decode_button, "3Dconnexion Space Explorer 3D Mouse"},
  { 0x046d, 0xc628, &HidApiInputDriver::hidapi_decode_axis, &HidApiInputDriver::hidapi_decode_button, "3Dconnexion Space Navigator for Notebooks"},
  { 0x046d, 0xc629, &HidApiInputDriver::hidapi_decode_axis, &HidApiInputDriver::hidapi_decode_button, "3Dconnexion SpacePilot Pro 3D Mouse"},
  { 0x046d, 0xc62b, &HidApiInputDriver::hidapi_decode_axis, &HidApiInputDriver::hidapi_decode_button, "3Dconnexion Space Mouse Pro"},
  { 0x256f, 0xc62e, &HidApiInputDriver::hidapi_decode_axis, &HidApiInputDriver::hidapi_decode_button, "3Dconnexion Space Mouse Wireless (cabled)"},
  { 0x256f, 0xc62f, &HidApiInputDriver::hidapi_decode_axis, &HidApiInputDriver::hidapi_decode_button, "3Dconnexion Space Mouse Wireless"},
  { 0x256f, 0xc631, &HidApiInputDriver::hidapi_decode_axis, &HidApiInputDriver::hidapi_decode_button, "3Dconnexion Space Mouse Pro Wireless (cabled)"},
  { 0x256f, 0xc632, &HidApiInputDriver::hidapi_decode_axis, &HidApiInputDriver::hidapi_decode_button, "3Dconnexion Space Mouse Pro Wireless"},
  { 0x256f, 0xc635, &HidApiInputDriver::hidapi_decode_axis, &HidApiInputDriver::hidapi_decode_button, "3Dconnexion Space Mouse Compact"},
  { 0x256f, 0xc63a, &HidApiInputDriver::hidapi_decode_axis, &HidApiInputDriver::hidapi_decode_button, "3Dconnexion Space Mouse Wireless BT"},
  // This is reported to be used with a 3Dconnexion Space Mouse Wireless 256f:c62e
  { 0x256f, 0xc652, &HidApiInputDriver::hidapi_decode_axis, &HidApiInputDriver::hidapi_decode_button, "3Dconnexion Universal Receiver"},
  { -1, -1, nullptr, nullptr, nullptr},
};

#define HIDAPI_LOG(f) hidapi_log(boost::format(f))
// NOLINTNEXTLINE(*macro-parentheses)
#define HIDAPI_LOGP(f, a) hidapi_log(boost::format(f) % a)
static void hidapi_log(const boost::format& format) {
  if (logstream) {
    const ch::system_clock::duration time = ch::system_clock::now() - logtime;

    logstream << ch::duration_cast<ch::milliseconds>(time).count() << ": " << format.str() << std::endl;
    if (logstream.tellp() > MAX_LOG_SIZE) {
      logstream.close();
    }
  }
}

static void hidapi_log_input(unsigned char *buf, int len)
{
  if (logstream) {
    std::ostringstream s;

    s << (boost::format("R: %1$2d/%1$02x:") % len).str();
    for (int idx = 0; idx < len; ++idx) {
      s << (boost::format(" %1$02x") % (int)buf[idx]).str();
    }
    HIDAPI_LOG(s.str());
  }
}

static std::string to_string(const wchar_t *wstr)
{
  if (wstr) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(wstr);
  }
  return "<null>";
}

static const device_id *match_device(const struct hid_device_info *info)
{
  for (int idx = 0; device_ids[idx].name != nullptr; ++idx) {
    if ((device_ids[idx].vendor_id == info->vendor_id) && (device_ids[idx].product_id == info->product_id)) {
      return &device_ids[idx];
    }
  }
  return nullptr;
}

HidApiInputDriver::HidApiInputDriver()
{
  name = "HidApiInputDriver";
}

void HidApiInputDriver::run()
{
  hidapi_input(hid_dev);
}

void HidApiInputDriver::hidapi_decode_axis(const unsigned char *buf, unsigned int len)
{
  if ((buf[0] == 1 || buf[0] == 2) && len == 7) {
    // Values are in the range -10..10 at min. speed and -2595..2595
    // at max. speed.
    const int16_t x_value = buf[1] | buf[2] << 8;
    const int16_t y_value = buf[3] | buf[4] << 8;
    const int16_t z_value = buf[5] | buf[6] << 8;
    if (x_value == 0 && y_value == 0 && z_value == 0) {
      return;
    }
    const double x = x_value / 350.0;
    const double y = y_value / 350.0;
    const double z = z_value / 350.0;
    if (buf[0] == 1) {
      InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(0, x));
      InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(1, y));
      InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(2, z));
    } else {
      InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(3, x));
      InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(4, y));
      InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(5, z));
    }
  } else if (buf[0] == 1 && len == 13) {
    // Same as above, but all 6 axis is a single 13 byte HID message.
    for (int a = 0; a < 6; ++a) {
      const int16_t i = buf[2 * a + 1] | (buf[2 * a + 2] << 8);
      double val = (double)i / 350.0;
      if (std::fabs(val) > 0.01) {
        InputEvent *event = new InputEventAxisChanged(a, val);
        InputDriverManager::instance()->sendEvent(event);
      }
    }
  }
}

void HidApiInputDriver::hidapi_decode_button(const unsigned char *buf, unsigned int len)
{
  if (buf[0] == 3 && len >= 3) {
    // Handle button events, on Linux the packet has a length of 3 but
    // on Windows the length is 13.
    const uint16_t current = buf[1] | buf[2] << 8;

    const std::bitset<16> bits_curr{current};
    const std::bitset<16> bits_last{buttons};

    for (int i = 0; i < 16; ++i) {
      if (bits_curr.test(i) != bits_last.test(i)) {
        InputEvent *event = new InputEventButtonChanged(i, bits_curr.test(i));
        InputDriverManager::instance()->sendEvent(event);
      }
    }

    buttons = current;
  }
}

void HidApiInputDriver::hidapi_input(hid_device *hid_dev)
{
  unsigned char buf[BUFLEN];
  unsigned int len;
  while ((len = hid_read(hid_dev, buf, BUFLEN)) > 0) {
    hidapi_log_input(buf, len);
    (this->*(dev->axis_decoder))(buf, len);
    (this->*(dev->button_decoder))(buf, len);
  }
  hid_close(hid_dev);
}

std::pair<hid_device *, const struct device_id *> HidApiInputDriver::enumerate() const
{
  hid_device *ret_hid_dev = nullptr;
  const struct device_id *ret_dev = nullptr;

  HIDAPI_LOG("Enumerating HID devices...");
  struct hid_device_info *info = hid_enumerate(0, 0);
  for (; info != nullptr; info = info->next) {
    HIDAPI_LOGP("D: %04x:%04x | path = %s, serial = %s, manufacturer = %s, product = %s",
                info->vendor_id % info->product_id % info->path
                % to_string(info->serial_number)
                % to_string(info->manufacturer_string)
                % to_string(info->product_string));
    const device_id *dev = match_device(info);
    if (!dev) {
      continue;
    }

    hid_device *hid_dev;

    HIDAPI_LOGP("P: %04x:%04x | %s", info->vendor_id % info->product_id % info->path);
    hid_dev = hid_open_path(info->path);

    if (!hid_dev) {
      HIDAPI_LOGP("O: %04x:%04x | %s", info->vendor_id % info->product_id % to_string(info->serial_number));
      hid_dev = hid_open(info->vendor_id, info->product_id, info->serial_number);
      if (!hid_dev) {
        continue;
      }
    }

    HIDAPI_LOGP("R: %04x:%04x | %s", info->vendor_id % info->product_id % to_string(info->serial_number));
    unsigned char buf[BUFLEN];
    const int len = hid_read_timeout(hid_dev, buf, BUFLEN, 100);
    HIDAPI_LOGP("?: %d", len);

    if (len < 0) {
      HIDAPI_LOGP("E: %s", to_string(hid_error(hid_dev)));
      hid_close(hid_dev);
      continue;
    }

    ret_dev = dev;
    ret_hid_dev = hid_dev;
    break;
  }
  hid_free_enumeration(info);
  HIDAPI_LOGP("Done enumerating (status = %s).", (ret_hid_dev != nullptr ? "ok" : "failed"));
  return {ret_hid_dev, ret_dev};
}

bool HidApiInputDriver::open()
{
  if (Settings::Settings::inputEnableDriverHIDAPILog.value()) {
    logtime = ch::system_clock::now();
    logstream.open(PlatformUtils::backupPath() + "/hidapi.log");
  }

  HIDAPI_LOG("HidApiInputDriver::open()");
  if (hid_init() < 0) {
    HIDAPI_LOG("hid_init() failed");
    PRINTD("Can't hid_init().\n");
    return false;
  }

  std::tie(this->hid_dev, this->dev) = enumerate();
  if (this->dev) {
    name = STR(std::setfill('0'), std::setw(4), std::hex,
               "HidApiInputDriver (", dev->vendor_id, ":", dev->product_id,
               " - ", dev->name, ")");
    start();
    HIDAPI_LOGP("HidApiInputDriver::open(): %s", name);
    return true;
  }

  HIDAPI_LOG("HidApiInputDriver::open(): No matching device found.");
  return false;
}

void HidApiInputDriver::close()
{
  this->dev = nullptr;
  this->hid_dev = nullptr;
  this->name = "HidApiInputDriver";
  HIDAPI_LOG("HidApiInputDriver::close()");
  logstream.close();
}

const std::string& HidApiInputDriver::get_name() const
{
  return name;
}

std::string HidApiInputDriver::get_info() const
{
  std::ostringstream stream;
  stream << get_name() << " ";
  if (isOpen()) {
    stream << "open" << " ";
    if (dev) {
      stream << "Vendor ID: " << dev->vendor_id << " ";
      stream << "Product ID: " << dev->product_id << " ";
    }
  } else {
    stream << "not open";
  }
  return stream.str();
}
