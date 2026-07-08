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

#include <bitset>
#include <boost/format.hpp>
#include <boost/nowide/convert.hpp>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>

#include "core/Settings.h"
#include "gui/input/InputDriverEvent.h"
#include "gui/input/InputDriverManager.h"
#include "platform/PlatformUtils.h"
#include "utils/printutils.h"

static constexpr int BUFLEN = 64;
static constexpr int MAX_LOG_SIZE = 100 * 1024 * 1024;

namespace ch = std::chrono;

static std::ofstream logstream;
static ch::system_clock::time_point logtime;

// http://www.linux-usb.org/usb.ids
// http://www.3dconnexion.eu/nc/service/faq/show_faq/7ece50ed-0b39-b57e-d3b2-4afd9420604e.html
static const std::vector<struct device_id> device_ids = {
  {0x046d, 0xc603, nullptr, &HidApiInputDriver::hidapi_sm_decode,
   "3Dconnexion Spacemouse Plus XT"},
  {0x046d, 0xc605, nullptr, &HidApiInputDriver::hidapi_sm_decode,
   "3Dconnexion CADman"},
  {0x046d, 0xc606, nullptr, &HidApiInputDriver::hidapi_sm_decode,
   "3Dconnexion Spacemouse Classic"},
  {0x046d, 0xc621, nullptr, &HidApiInputDriver::hidapi_sm_decode,
   "3Dconnexion Spaceball 5000"},
  {0x046d, 0xc623, nullptr, &HidApiInputDriver::hidapi_sm_decode,
   "3Dconnexion Space Traveller 3D Mouse"},
  {0x046d, 0xc625, nullptr, &HidApiInputDriver::hidapi_sm_decode,
   "3Dconnexion Space Pilot 3D Mouse"},
  {0x046d, 0xc626, nullptr, &HidApiInputDriver::hidapi_sm_decode,
   "3Dconnexion Space Navigator 3D Mouse"},
  {0x046d, 0xc627, nullptr, &HidApiInputDriver::hidapi_sm_decode,
   "3Dconnexion Space Explorer 3D Mouse"},
  {0x046d, 0xc628, nullptr, &HidApiInputDriver::hidapi_sm_decode,
   "3Dconnexion Space Navigator for Notebooks"},
  {0x046d, 0xc629, nullptr, &HidApiInputDriver::hidapi_sm_decode,
   "3Dconnexion SpacePilot Pro 3D Mouse"},
  {0x046d, 0xc62b, nullptr, &HidApiInputDriver::hidapi_sm_decode,
   "3Dconnexion Space Mouse Pro"},
  {0x256f, 0xc62e, nullptr, &HidApiInputDriver::hidapi_sm_decode,
   "3Dconnexion Space Mouse Wireless (cabled)"},
  {0x256f, 0xc62f, nullptr, &HidApiInputDriver::hidapi_sm_decode,
   "3Dconnexion Space Mouse Wireless"},
  {0x256f, 0xc631, nullptr, &HidApiInputDriver::hidapi_sm_decode,
   "3Dconnexion Space Mouse Pro Wireless (cabled)"},
  {0x256f, 0xc632, nullptr, &HidApiInputDriver::hidapi_sm_decode,
   "3Dconnexion Space Mouse Pro Wireless"},
//  {0x256f, 0xc635, nullptr, &HidApiInputDriver::hidapi_sm_decode,
//   "3Dconnexion Space Mouse Compact"},
  {0x256f, 0xc635, &HidApiInputDriver::hidapi_generic_init, &HidApiInputDriver::hidapi_generic_decode,
   "3Dconnexion Space Mouse Compact"},
  {0x256f, 0xc63a, nullptr, &HidApiInputDriver::hidapi_sm_decode,
   "3Dconnexion Space Mouse Wireless BT"},
  // This is reported to be used with a 3Dconnexion Space Mouse Wireless 256f:c62e
  {0x256f, 0xc652, nullptr, &HidApiInputDriver::hidapi_sm_decode,
   "3Dconnexion Universal Receiver"},
  {0x046d, 0xc21d, &HidApiInputDriver::hidapi_generic_init, &HidApiInputDriver::hidapi_generic_decode,
   "Logitech Gamepad F310"},
  {0x046d, 0xc339, &HidApiInputDriver::hidapi_generic_init, &HidApiInputDriver::hidapi_generic_decode,
   "Logitech G Pro TKL keyboard"},
};

struct descriptorPatch {
  int vendor_id;
  int product_id;
  const std::string bad;
  const std::string fixed;
};

#define PAGE(n)           "05" #n
#define USAGE(n)          "09" #n
#define COLL(n)           "a1" #n
#define LMIN1(a)          "15" #a
#define LMAX1(a)          "25" #a
#define LMAX3(a, b, c)    LMAX4(a, b, c, 00)
#define LMAX4(a, b, c, d) "27" #a #b #c #d
#define PMIN1(a)          "35" #a
#define PMAX1(a)          "45" #a
#define PMAX2(a, b)       "46" #a #b
#define PMAX3(a, b, c)    PMAX4(a, b, c, 00)
#define PMAX4(a, b, c, d) "47" #a #b #c #d
#define RSIZE(n)          "75" #n
#define RCOUNT(n)         "95" #n
#define INPUT(n)          "81" #n
#define ENDCOLL()         "c0"
#define UMIN(n)           "19" #n
#define UMAX(n)           "29" #n
#define UNIT(n)           "65" #n

static const std::vector<struct descriptorPatch> patches = {
  { // Logitech Gamepad F310
    0x046d, 0xc21d,
    /* bad */
    PAGE(01) USAGE(05)
    COLL(01)
      USAGE(00)
      COLL(00)
        USAGE(30) USAGE(31) LMIN1(00) LMAX1(ff) PMIN1(00) PMAX1(ff)
        RSIZE(10) RCOUNT(02) INPUT(02)
      ENDCOLL()
      USAGE(00)
      COLL(00)
        USAGE(33) USAGE(34) LMIN1(00) LMAX1(ff) RSIZE(10) RCOUNT(02) INPUT(02)
      ENDCOLL()
      USAGE(00)
      COLL(00)
        USAGE(32) LMIN1(00) LMAX1(ff) RSIZE(10) RCOUNT(01) INPUT(02)
      ENDCOLL()
      PAGE(09) UMIN(01) UMAX(0a) LMIN1(00) LMAX1(01) RSIZE(01) RCOUNT(0a) PMAX1(00) INPUT(02)
      PAGE(01) USAGE(39) LMIN1(01) LMAX1(08) PMIN1(00) PMAX2(3b, 10) UNIT(0e) RSIZE(04) RCOUNT(01) INPUT(42)
      RSIZE(02) RCOUNT(01) INPUT(03)
      RSIZE(08) RCOUNT(02) INPUT(03)
    ENDCOLL(),
    /* fix */
    PAGE(01) USAGE(05)
    COLL(01)
      USAGE(00)
      COLL(00)
        USAGE(30) USAGE(31) LMIN1(00) LMAX3(ff, ff, 00) PMIN1(00) PMAX3(ff, ff, 00)
        RSIZE(10) RCOUNT(02) INPUT(02)
      ENDCOLL()
      USAGE(00)
      COLL(00)
        USAGE(33) USAGE(34) LMIN1(00) LMAX3(ff, ff, 00) RSIZE(10) RCOUNT(02) INPUT(02)
      ENDCOLL()
      USAGE(00)
      COLL(00)
        USAGE(32) LMIN1(00) LMAX3(ff, ff, 00) RSIZE(10) RCOUNT(01) INPUT(02)
      ENDCOLL()
      PAGE(09) UMIN(01) UMAX(0a) LMIN1(00) LMAX1(01) RSIZE(01) RCOUNT(0a) PMAX1(00) INPUT(02)
      PAGE(01) USAGE(39) LMIN1(01) LMAX1(08) PMIN1(00) PMAX2(3b, 01) UNIT(14) RSIZE(04) RCOUNT(01) INPUT(42)
      RSIZE(02) RCOUNT(01) INPUT(03)
      RSIZE(08) RCOUNT(02) INPUT(03)
    ENDCOLL(),
  }
};

#define HIDAPI_LOG(f) hidapi_log(boost::format(f))
// NOLINTNEXTLINE(*macro-parentheses)
#define HIDAPI_LOGP(f, a) hidapi_log(boost::format(f) % a)
static void hidapi_log(const boost::format& format)
{
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
    std::string header = (boost::format("R: %1$2d/%1$02x:") % len).str();

    for (int i = 0; i < len; i += 16) {
      std::ostringstream s;

      if (i == 0) {
        s << header;
      } else {
        std::string offset = (boost::format("%1$04x:") % i).str();
        for (int k = offset.size(); k < header.size(); k++) {
          s << " ";
        }
        s << offset;
      }
      for (int j = 0; j < 16 && i + j < len; j++) {
        s << (boost::format(" %1$02x") % (int)buf[i + j]).str();
      }
      HIDAPI_LOG(s.str());
    }
  }
}

static std::string to_string(const wchar_t *wstr)
{
  return wstr ? boost::nowide::narrow(wstr) : "<null>";
}

static const device_id *match_device(const struct hid_device_info *info)
{
  for (const struct device_id& id : device_ids) {
    if ((id.vendor_id == info->vendor_id) &&
        (id.product_id == info->product_id)) {
      return &id;
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

void HidApiInputDriver::hidapi_sm_decode(const unsigned char *buf, unsigned int len)
{
  hidapi_sm_decode_axis(buf, len);
  hidapi_sm_decode_button(buf, len);
}

void HidApiInputDriver::hidapi_sm_decode_axis(const unsigned char *buf, unsigned int len)
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

void HidApiInputDriver::hidapi_sm_decode_button(const unsigned char *buf, unsigned int len)
{
  if (buf[0] == 3 && len >= 3) {
    // Handle button events, on Linux the packet has a length of 3 but
    // on Windows the length is 13.
    const uint16_t current = buf[1] | buf[2] << 8;

    const std::bitset<16> bits_curr{current};
    const std::bitset<16> bits_last{buttonState};  // NOLINT

    for (int i = 0; i < 16; ++i) {
      if (bits_curr.test(i) != bits_last.test(i)) {
        InputEvent *event = new InputEventButtonChanged(i, bits_curr.test(i));
        InputDriverManager::instance()->sendEvent(event);
      }
    }

    buttonState = current;
  }
}

void HidApiInputDriver::hidapi_input(hid_device *hid_dev)
{
  unsigned char buf[BUFLEN];
  unsigned int len;
  while ((len = hid_read(hid_dev, buf, BUFLEN)) > 0) {
    hidapi_log_input(buf, len);
    (this->*(dev->decode))(buf, len);
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
                info->vendor_id % info->product_id % info->path % to_string(info->serial_number) %
                  to_string(info->manufacturer_string) % to_string(info->product_string));
    const device_id *dev = match_device(info);
    if (!dev) {
      continue;
    }

    hid_device *hid_dev;

    HIDAPI_LOGP("P: %04x:%04x | %s", info->vendor_id % info->product_id % info->path);
    hid_dev = hid_open_path(info->path);

    if (!hid_dev) {
      HIDAPI_LOGP("O: %04x:%04x | %s",
                  info->vendor_id % info->product_id % to_string(info->serial_number));
      hid_dev = hid_open(info->vendor_id, info->product_id, info->serial_number);
      if (!hid_dev) {
        continue;
      }
    }

    HIDAPI_LOGP("R: %04x:%04x | %s",
                info->vendor_id % info->product_id % to_string(info->serial_number));
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
    logstream.open(PlatformUtils::backupPath() + "/hidapi.log", std::ios_base::app);
  }

  HIDAPI_LOG("HidApiInputDriver::open()");
  if (hid_init() < 0) {
    HIDAPI_LOG("hid_init() failed");
    PRINTD("Can't hid_init().\n");
    return false;
  }

  std::tie(this->hid_dev, this->dev) = enumerate();
  if (this->dev) {

    if (this->dev->init) {
      (this->*(dev->init))();
    }

    name = STR(std::setfill('0'), std::setw(4), std::hex, "HidApiInputDriver (", dev->vendor_id, ":",
               dev->product_id, " - ", dev->name, ")");
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
  std::string s = get_name();
  if (isOpen()) {
    s += "open ";
    if (dev) {
      s += (boost::format("Vendor ID: %04x Product ID: %04x") % dev->vendor_id % dev->product_id)
        .str();
    }
  } else {
    s += "not open";
  }
  return s;
}

void HidApiInputDriver::hidapi_generic_init()
{
  generic_loadDescriptor();
}

class HidGlobal {
public:
  int usagePage{-1};
  int id{0};
  struct {
    int minimum{0};
    int smaximum{0};
    unsigned int umaximum{0};
  } logical;
  struct {
    int minimum{0};
    int smaximum{0};
    unsigned int umaximum{0};
  } physical;
  struct {
    int size{0};
    int count{0};
  } report;
};

class HidLocal {
public:
 std::vector<int> usages;
 int usageMinimum{-1};
 int usageMaximum{-1};
};

class UsageInfo {
public:
  virtual void decode(HidApiInputDriver *haid, const uint8_t *report, size_t size);
protected:
  UsageInfo(int offsetBits, const HidGlobal& global) :
    offsetBits(offsetBits), nbits(global.report.size) {};
  int offsetBits;
  int nbits;
};

class UsageInfoHat;

class UsageInfoButton : public UsageInfo {
public:
  UsageInfoButton(int offsetBits, const HidGlobal& global) : UsageInfo(offsetBits, global) {
    index = indexesUsed++;
    HIDAPI_LOGP("      0x%04x+%d/%d: button %d", (offsetBits/8) % (offsetBits & 0x07) % nbits % index);
  };
  void decode(HidApiInputDriver *haid, const uint8_t *report, size_t size) override
  {
    bool newState = (report[offsetBits/8] & (1 << (offsetBits & 0x07))) != 0;
    if (newState != oldState) {
      HIDAPI_LOGP("button %d %s", index % (newState ? "pressed" : "released"));
      InputEvent *event = new InputEventButtonChanged(index, newState);
      InputDriverManager::instance()->sendEvent(event);
      oldState = newState;
    }
  };

protected:
  static int indexesUsed;

private:
  bool oldState{false};
  int index;

  friend UsageInfoHat;
};

int UsageInfoButton::indexesUsed = 0;

static int getBits(const uint8_t *buf, int index, int n)
{
  int val;
  int firstByteIndex = index/8;
  int firstByteBits = 8 - (index & 0x07);
  if (firstByteBits >= n) {
    return (buf[firstByteIndex] >> (index & 0x07)) & ((1 << n) - 1);
  }
  val = buf[firstByteIndex] >> (index & 0x07);
  index += firstByteBits;
  n -= firstByteBits;
  int bitsSoFar = firstByteBits;

  while (n >= 8) {
    val |= buf[index/8] << bitsSoFar;
    index += 8;
    n -= 8;
    bitsSoFar += 8;
  }

  if (n > 0) {
    val |= (buf[index/8] & ((1 << n) - 1)) << bitsSoFar;
  }

  return val;
}

class UsageInfoLinear : public UsageInfo {
public:
  UsageInfoLinear(int offsetBits, const HidGlobal& global) : UsageInfo(offsetBits, global) {
    signExtend = (global.logical.minimum < 0);
    // Work around a bug in some descriptors.  See the comments for Logical Maximum in the parser.
    int maximum = global.logical.smaximum >= global.logical.minimum
      ? global.logical.smaximum : global.logical.umaximum;

    // The mapping here is arguably not right.  OpenSCAD wants an axis to run from -1 to +1,
    // but for HID there can be arbitrary limits and they can be asymmetrical - it could run
    // from -50 to +100, or from +100 to +200.  Here we just map the minimum to -1 and the maximum
    // to +1.
    minimum = global.logical.minimum;
    scale = 2.0 / (maximum - minimum);
    offset = -1.0;

    index = indexesUsed++;
    HIDAPI_LOGP("      0x%04x+%d/%d: linear %d %d:%d scale=%g",
                (offsetBits/8) % (offsetBits & 0x07) % nbits % index % minimum % maximum % scale);
  };
  void decode(HidApiInputDriver *haid, const uint8_t *report, size_t size) override
  {
    int newVal = getBits(report, offsetBits, nbits);
    // Sign extend if appropriate.
    if (signExtend && newVal >> (nbits-1)) {
      newVal |= -1 << nbits;
    }
    if (newVal != oldVal) {
      double fVal = (newVal - minimum) * scale + offset;
      HIDAPI_LOGP("linear %d raw=%d val=%g", index % newVal % fVal);
      InputEvent *event = new InputEventAxisChanged(index, fVal);
      InputDriverManager::instance()->sendEvent(event);
      oldVal = newVal;
    }
  };

protected:
  static int indexesUsed;

private:
  int minimum;
  double scale;
  double offset;
  bool signExtend;
  int index;
  int oldVal{0};

  friend UsageInfoHat;
};

int UsageInfoLinear::indexesUsed = 0;

// Hat switches pose a problem.  I come up with four(!) non-silly ways to model them.
// 1)  As eight mutually-exclusive buttons.
// 2)  As four buttons that can be combined in eight ways.
// 3)  As a low-resolution joystick (x={-1, 0, 1}, y={-1, 0, 1})
// 4)  As an angle indicator (null, 0, 45, 90, ..., 270, 315).
//
// #4, an angle indicator, is what it's encoded as in HID, but I can't come up with any useful
// way to map that to OpenSCAD actions.
//
// Answer:  All of the above!  Well, except for 4.  Report events for 8+4 buttons plus a joystick.
// You can map whichever one(s) you want.
class UsageInfoHat : public UsageInfo {
public:
  UsageInfoHat(int offsetBits, const HidGlobal& global) : UsageInfo(offsetBits, global) {
    indexButton8 = UsageInfoButton::indexesUsed;
    UsageInfoButton::indexesUsed += 8;

    indexButton4 = UsageInfoButton::indexesUsed;
    UsageInfoButton::indexesUsed += 4;

    indexLinear = UsageInfoLinear::indexesUsed;
    UsageInfoLinear::indexesUsed += 2;

    HIDAPI_LOGP("      0x%04x+%d/%d: hat B%d-%d, B%d-%d, L%d",
      (offsetBits/8)
      % (offsetBits & 0x07)
      % nbits % indexButton8
      % (indexButton8 + 7)
      % indexButton4
      % (indexButton4 + 3)
      % indexLinear);
  }
  void decode(HidApiInputDriver *haid, const uint8_t *report, size_t size) override
  {
    int newVal = getBits(report, offsetBits, nbits);
    if (newVal != oldVal) {
      // First, release and then press the appropriate 8-way buttons.
      if (oldVal != 0)  {
        HIDAPI_LOGP("hat: button8 %d released", oldVal);
        InputEvent *releaseButton8 = new InputEventButtonChanged(indexButton8 + oldVal - 1, false);
        InputDriverManager::instance()->sendEvent(releaseButton8);
      }
      if (newVal != 0) {
        HIDAPI_LOGP("hat: button8 %d pressed", newVal);
        InputEvent *pressButton8 = new InputEventButtonChanged(indexButton8 + newVal - 1, true);
        InputDriverManager::instance()->sendEvent(pressButton8);
      }

      // Next, change the states of the appropriate 4-way buttons.
      const uint8_t buttons4[] = { 0x00, 0x01, 0x03, 0x02, 0x06, 0x04, 0x0c, 0x08, 0x09 };
      uint8_t oldButton4 = buttons4[oldVal];
      uint8_t newButton4 = buttons4[newVal];
      for (int i = 0; i < 4; i++) {
        if ((oldButton4 ^ newButton4) & (1 << i)) {
          bool newState = ((newButton4 & (1 <<i)) != 0);
          HIDAPI_LOGP("hat: button4 %d %s", i % (newState ? "pressed" : "released"));
          InputEvent *button4Event = new InputEventButtonChanged(indexButton4 + i, newState);
          InputDriverManager::instance()->sendEvent(button4Event);
        }
      }

      // Finally, report as a joystick.
      double x = 0.0;
      double y = 0.0;
      switch(newVal) {
      case 0: break;
      case 1: x = 0.0; y = 1.0; break;
      case 2: x = 1.0; y = 1.0; break;
      case 3: x = 1.0; y = 0.0; break;
      case 4: x = 1.0; y = -1.0; break;
      case 5: x = 0.0; y = -1.0; break;
      case 6: x = -1.0; y = -1.0; break;
      case 7: x = -1.0; y = 0.0; break;
      case 8: x = -1.0; y = 1.0; break;
      }
      HIDAPI_LOGP("hat: joystick %g, %g", x % y);
      InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(indexLinear + 0, x));
      InputDriverManager::instance()->sendEvent(new InputEventAxisChanged(indexLinear + 1, y));

      oldVal = newVal;
    }
  };
private:
  int indexButton8;
  int indexButton4;
  int indexLinear;
  int oldVal{0};
};

enum {
  UP_DESKTOP = 0x01,
  // ...
  UP_BUTTON = 0x09,
};

enum {
  UP_DESKTOP_X          = 0x30,
  UP_DESKTOP_Y          = 0x31,
  UP_DESKTOP_Z          = 0x32,
  UP_DESKTOP_RX         = 0x33,
  UP_DESKTOP_RY         = 0x34,
  UP_DESKTOP_RZ         = 0x35,
  UP_DESKTOP_HAT        = 0x39,
  UP_DESKTOP_DPAD_UP    = 0x90,
  UP_DESKTOP_DPAD_DOWN  = 0x91,
  UP_DESKTOP_DPAD_RIGHT = 0x92,
  UP_DESKTOP_DPAD_LEFT  = 0x93,
};

static std::shared_ptr<UsageInfo> usageInfoFactory(int page, int usage, int offsetBits,
  const HidGlobal& global)
{
  enum {
    UNKNOWN,
    BUTTON,
    LINEAR,
    HAT,
  } type = UNKNOWN;

  switch (page) {
  case UP_DESKTOP:
    switch (usage) {
    case UP_DESKTOP_DPAD_UP:
    case UP_DESKTOP_DPAD_DOWN:
    case UP_DESKTOP_DPAD_RIGHT:
    case UP_DESKTOP_DPAD_LEFT:
      type = BUTTON;
      break;
    case UP_DESKTOP_X:
    case UP_DESKTOP_Y:
    case UP_DESKTOP_Z:
    case UP_DESKTOP_RX:
    case UP_DESKTOP_RY:
    case UP_DESKTOP_RZ:
      type = LINEAR;
      break;
    case UP_DESKTOP_HAT:
      type = HAT;
      break;
    default:
      break;
    }
    break;
  case UP_BUTTON:
    // NEEDSWORK?  We assign button numbers by the order in the descriptor, ignoring what button
    // number the usage says it is.
    type = BUTTON;
    break;
  }
  std::shared_ptr<UsageInfo> ui;
  switch (type) {
  case BUTTON:
    ui = std::make_shared<UsageInfoButton>(offsetBits, global);
    break;
  case LINEAR:
    ui = std::make_shared<UsageInfoLinear>(offsetBits, global);
    break;
  case HAT:
    ui = std::make_shared<UsageInfoHat>(offsetBits, global);
    break;
  default:
    break;
  }
  return ui;
}

uint8_t HidApiInputDriver::hexNibble(char c)
{
    assert(isxdigit(c));
    return isdigit(c)
      ? c - '0'
      : isupper(c)
      ? c - 'A' + 10
      : c - 'a' + 10;
}

bool HidApiInputDriver::matchDescriptor(const std::string& s, uint8_t *buf, size_t len)
{
  for (size_t i = 0; i < s.size(); i++) {
    if (isspace(s[i])) {
      continue;
    }
    if (len == 0) {
      return (false);
    }
    uint8_t n1 = hexNibble(s[i]);
    i++;
    assert(i < s.size());
    uint8_t b = (n1 << 4) | hexNibble(s[i]);
    if (*buf != b) {
      return false;
    }
    buf++;
    len--;
  }
  if (len != 0) {
    return false;
  }
  return true;
}

void HidApiInputDriver::doPatch(const std::string& s, uint8_t *buf, size_t *len)
{
  *len = 0;
  for (size_t i = 0; i < s.size(); i++) {
    if (isspace(s[i])) {
      continue;
    }
    uint8_t n1 = hexNibble(s[i]);
    i++;
    assert(i < s.size());
    uint8_t b = (n1 << 4) | hexNibble(s[i]);
    *buf = b;
    buf++;
    (*len)++;
  }
}

void HidApiInputDriver::patch(uint8_t *buf, size_t *len)
{
  for (const struct descriptorPatch& p : patches) {
    if (p.vendor_id == dev->vendor_id
      && p.product_id == dev->product_id
      && matchDescriptor(p.bad, buf, *len)) {
      HIDAPI_LOG("Patching...");
      doPatch(p.fixed, buf, len);
      break;
    }
  }
}

void HidApiInputDriver::generic_loadDescriptor()
{
  HidGlobal global;
  HidLocal local;
  const HidLocal local0;
  std::vector<HidGlobal> stack;
  unsigned long offsetBits = 0;

  uint8_t descriptor[HID_API_MAX_REPORT_DESCRIPTOR_SIZE];
  ssize_t tmpSize = hid_get_report_descriptor(this->hid_dev, descriptor, sizeof (descriptor));
  if (tmpSize < 0) {
    HIDAPI_LOG("hid_get_report_descriptor failed");
    throw HidException(_("Failed retrieving HID report descriptor"));
  }
  size_t descriptorSize = tmpSize;

  HIDAPI_LOG("Descriptor:");
  hidapi_log_input(descriptor, descriptorSize);

  patch(descriptor, &descriptorSize);

  int len;
  for (size_t i = 0; i < descriptorSize; i += len) {
    int bSize = descriptor[i] & 0x03;
    if (bSize == 3) {
      bSize = 4;
    }
    int bType = (descriptor[i] & 0x0c) >> 2;
    assert(bType >= 0 && bType < 4);
    int bTag = (descriptor[i] & 0xf0) >> 4;
    uint8_t *data;
    int bDataSize;
    int tag;
    if (bTag == 0x0f) {
      bDataSize = descriptor[i+1];
      int bLongItemTag = descriptor[i+2];
      data = descriptor + i + 3;
      len = bDataSize + 3;
      tag = bLongItemTag;
    } else {
      bDataSize = bSize;
      len = bSize + 1;
      data = descriptor + i + 1;
      tag = bTag;
    }

    // Pick up a data item up to 32 bits, if any.  If actual data item is longer than that,
    // or is best not viewed as a single integer, you're on your own.
    uint32_t uDataVal = 0;
    if (bSize > 0) {
      uDataVal |= data[0];
    }
    if (bSize > 1) {
      uDataVal |= data[1] << 8;
    }
    if (bSize > 2) {
      uDataVal |= data[2] << 16;
    }
    if (bSize > 3) {
      uDataVal |= data[3] << 24;
    }
    int32_t sDataVal;
    // Calculate the value as both a signed integer and an unsigned integer.  See the comments
    // on Logical Maximum.
    if (bSize < 4 && (uDataVal & (1 << (bSize*8 - 1)))) {
      sDataVal = uDataVal | (-1 << bSize*8);
    } else {
      sDataVal = uDataVal;
    }

    enum { HID_TYPE_MAIN=0, HID_TYPE_GLOBAL=1, HID_TYPE_LOCAL=2, HID_TYPE_RESERVED=3 };
    static const char *types[] = { "Main", "Glob", "Locl", "Rsvd" };
    std::string msg = (boost::format("%04zx: %-4s") % i % types[bType]).str();

    switch (bType) {
      case HID_TYPE_MAIN:
        enum {
          HID_MAIN_INPUT = 0x08,
          HID_MAIN_OUTPUT = 0x09,
          HID_MAIN_FEATURE = 0x0b,
          HID_MAIN_COLLECTION = 0x0a,
          HID_MAIN_ENDCOLLECTION = 0x0c
        };
        switch (tag) {
          case HID_MAIN_INPUT:
            HIDAPI_LOGP("%s Input 0x%02x", msg % uDataVal);
            enum {
              HID_INPUT_CONSTANT    = 0x00000001,   // vs DATA
              HID_INPUT_VARIABLE    = 0x00000002,   // vs ARRAY
              HID_INPUT_RELATIVE    = 0x00000004,   // vs ABSOLUTE
              HID_INPUT_WRAP        = 0x00000008,
              HID_INPUT_NONLINEAR   = 0x00000010,
              HID_INPUT_NOPREFERRED = 0x00000020,
              HID_INPUT_NULLSTATE   = 0x00000040,
              // reserved             0x00000080,
              HID_INPUT_BUFFERED    = 0x00000100,
            };
            if (uDataVal & HID_INPUT_CONSTANT) {
              offsetBits += global.report.size * global.report.count;
              break;
            }
            if (!(uDataVal & HID_INPUT_VARIABLE)) {
              // No support for ARRAY yet.
              offsetBits += global.report.size * global.report.count;
              break;
            }
            if (uDataVal & HID_INPUT_RELATIVE) {
              // NEEDSWORK
              // SpaceMouse appears to report XYZ and RXRYRZ as relative, while the Logitech
              // gamepad reports them as absolute.
              // But:  is a joystick-like report absolute (because it tells you the absolute
              // joystick deflection), or is it relative (because it tells you to move in a
              // particular direction at a particular rate)?  I would say absolute, because
              // it has a zero point that it returns to, while a mouse does not.
              //
              // So for now we'll just ignore the bit, which will mean that if we ever try
              // to handle an actual relative device (like a mouse) nothing good will happen.

              // offsetBits += global.report.size * global.report.count;
              // break;
            }

            if (local.usages.empty()) {
              assert(local.usageMinimum >= 0 && local.usageMaximum >= 0);
              assert(local.usageMaximum >= local.usageMinimum);
              for (int i = local.usageMinimum; i <= local.usageMaximum; i++) {
                local.usages.push_back(i);
              }
            } else {
              assert(local.usageMinimum < 0 && local.usageMaximum < 0);
            }
            assert(local.usages.size() == global.report.count);
            for (int u : local.usages) {
              std::shared_ptr<UsageInfo> ui =
                usageInfoFactory(global.usagePage, u, offsetBits, global);
              if (ui) {
                items[global.id].push_back(ui);
              } else {
                HIDAPI_LOGP("No mapping for usage 0x%02x/0x%02x", global.usagePage % u);
              }
              offsetBits += global.report.size;
            }
            break;
          case HID_MAIN_OUTPUT:
            HIDAPI_LOGP("%s Output 0x%02x", msg % uDataVal);
            break;
          case HID_MAIN_FEATURE:
            HIDAPI_LOGP("%s Feature 0x%02x", msg % uDataVal);
            break;
          case HID_MAIN_COLLECTION:
            HIDAPI_LOGP("%s Collection 0x%02x", msg % uDataVal);
            break;
          case HID_MAIN_ENDCOLLECTION:
            HIDAPI_LOGP("%s End collection", msg);
            break;
          default:
            HIDAPI_LOGP("%s tag 0x%02x", msg % tag);
            break;
        }
        local = local0;
        break;
      case HID_TYPE_GLOBAL:
        enum {
          HID_GLOBAL_USAGE_PAGE = 0x00,
          HID_GLOBAL_LMIN = 0x01,
          HID_GLOBAL_LMAX = 0x02,
          HID_GLOBAL_PMIN = 0x03,
          HID_GLOBAL_PMAX = 0x04,
          HID_GLOBAL_UNIT_EXP = 0x05,
          HID_GLOBAL_UNIT = 0x06,
          HID_GLOBAL_REPORT_SIZE = 0x07,
          HID_GLOBAL_REPORT_ID = 0x08,
          HID_GLOBAL_REPORT_COUNT = 0x09,
          HID_GLOBAL_PUSH = 0x0a,
          HID_GLOBAL_POP = 0x0b,
        };
        switch (tag) {
          case HID_GLOBAL_USAGE_PAGE:
            HIDAPI_LOGP("%s Usage page 0x%02x", msg % uDataVal);
            global.usagePage = uDataVal;
            break;
          case HID_GLOBAL_LMIN:
            HIDAPI_LOGP("%s Logical minimum 0x%02x", msg % uDataVal);
            global.logical.minimum = sDataVal;
            break;
          case HID_GLOBAL_LMAX:
            // Work around a bug in some descriptors  where the maximum is erroneously treated
            // as an unsigned value.  When we actually go to use it, if the maximum is less than
            // the minimum, we assume that it should have been treated as unsigned.
            //
            // Example:  XBox 360 controller.  (Ref https://github.com/DJm00n/ControllersInfo)
            // It reports a two-byte FFFF, which should be treated as -1 but the intent is 65535.
            //
            // Example: Logitech Gamepad F310.  It reports a single-byte FF, which should be
            // treated as -1.  That error is compounded because the actual maximum is FFFF.
            // (It is possible that this is the result of misreading a report like the one for
            // the XBox 360 above, where the program says "your two-byte FFFF could have been
            // a one-byte FF"... and missing the fact that -1 was the wrong answer in both
            // cases.)
            HIDAPI_LOGP("%s Logical maximum 0x%02x", msg % uDataVal);
            global.logical.smaximum = sDataVal;
            global.logical.umaximum = uDataVal;
            break;
          case HID_GLOBAL_PMIN:
            HIDAPI_LOGP("%s Physical minimum 0x%02x", msg % uDataVal);
            global.physical.minimum = sDataVal;
            break;
          case HID_GLOBAL_PMAX:
            // The devices mentioned above for Logical Max have the same flaw for Physical Max.
            HIDAPI_LOGP("%s Physical maximum 0x%02x", msg % uDataVal);
            global.physical.smaximum = sDataVal;
            global.physical.umaximum = uDataVal;
            break;
          case HID_GLOBAL_UNIT_EXP:
            HIDAPI_LOGP("%s Unit exponent 0x%02x", msg % uDataVal);
            // No support for units yet.
            break;
          case HID_GLOBAL_UNIT:
            HIDAPI_LOGP("%s Unit 0x%02x", msg % uDataVal);
            // No support for units yet.
            break;
          case HID_GLOBAL_REPORT_SIZE:
            HIDAPI_LOGP("%s Report size 0x%02x", msg % uDataVal);
            global.report.size = uDataVal;
            break;
          case HID_GLOBAL_REPORT_ID:
            HIDAPI_LOGP("%s Report ID 0x%02x", msg % uDataVal);
            global.id = uDataVal;
            offsetBits = 8;
            hasReportIDs = true;
            break;
          case HID_GLOBAL_REPORT_COUNT:
            HIDAPI_LOGP("%s Report count %d", msg % uDataVal);
            global.report.count = uDataVal;
            break;
          case HID_GLOBAL_PUSH:
            HIDAPI_LOGP("%s Push", msg);
            stack.push_back(global);
            break;
          case HID_GLOBAL_POP:
            HIDAPI_LOGP("%s Pop", msg);
            global = stack.back();
            stack.pop_back();
            break;
          default:
            HIDAPI_LOGP("%s tag 0x%02x", msg % tag);
            hidapi_log_input(data, bDataSize);
            break;
        }
        break;
      case HID_TYPE_LOCAL:
        enum {
          HID_LOCAL_USAGE = 0x00,
          HID_LOCAL_USAGE_MIN = 0x01,
          HID_LOCAL_USAGE_MAX = 0x02,
          HID_LOCAL_D_INDEX = 0x03,
          HID_LOCAL_D_MIN = 0x04,
          HID_LOCAL_D_MAX = 0x05,
          HID_LOCAL_S_INDEX = 0x07,
          HID_LOCAL_S_MIN = 0x08,
          HID_LOCAL_S_MAX = 0x09,
          HID_LOCAL_DELIMITER = 0x0a,
        };
        switch (tag) {
          case HID_LOCAL_USAGE:
            HIDAPI_LOGP("%s Usage 0x%02x, %02x", msg % global.usagePage % uDataVal);
            local.usages.push_back(uDataVal);
            break;
          case HID_LOCAL_USAGE_MIN:
            HIDAPI_LOGP("%s Usage min 0x%02x, %02x", msg % global.usagePage % uDataVal);
            local.usageMinimum = uDataVal;
            break;
          case HID_LOCAL_USAGE_MAX:
            HIDAPI_LOGP("%s Usage max 0x%02x, %02x", msg % global.usagePage % uDataVal);
            local.usageMaximum = uDataVal;
            break;
          case HID_LOCAL_D_INDEX:
            HIDAPI_LOGP("%s Designator index 0x%02x", msg % uDataVal);
            // Not supported
            break;
          case HID_LOCAL_D_MIN:
            HIDAPI_LOGP("%s Designator min 0x%02x", msg % uDataVal);
            // Not supported
            break;
          case HID_LOCAL_D_MAX:
            HIDAPI_LOGP("%s Designator max 0x%02x", msg % uDataVal);
            // Not supported
            break;
          case HID_LOCAL_S_INDEX:
            HIDAPI_LOGP("%s String index %02x", msg % uDataVal);
            // Not supported
            break;
          case HID_LOCAL_S_MIN:
            HIDAPI_LOGP("%s String min %02x", msg % uDataVal);
            // Not supported
            break;
          case HID_LOCAL_S_MAX:
            HIDAPI_LOGP("%s String max %02x", msg % uDataVal);
            // Not supported
            break;
          case HID_LOCAL_DELIMITER:
            HIDAPI_LOGP("%s Delimiter %d", msg % uDataVal);
            // Not supported
            break;
          default:
            HIDAPI_LOGP("%s tag 0x%02x", msg % tag);
            hidapi_log_input(data, bDataSize);
            break;
        }
        break;
      case HID_TYPE_RESERVED:
        HIDAPI_LOGP("%s tag 0x%02x", msg % types[bType] % tag);
        hidapi_log_input(data, bDataSize);
        break;
    }
  }
}

void HidApiInputDriver::hidapi_generic_decode(const unsigned char *buf, unsigned int len)
{
  const uint8_t reportID = hasReportIDs ? buf[0] : 0;
  for (std::shared_ptr<UsageInfo> u : items[reportID]) {
    u->decode(this, buf, len);
  }
}
