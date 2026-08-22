#!/usr/bin/env python3

"""Generate src/core/UnicodeIdentifierTables.h from the Unicode Character Database.

The generated tables describe the OpenSCAD identifier profile, which is the
default identifier syntax of UAX #31 restricted by the general security profile
of UTS #39:

    ID_Start    := (XID_Start ∪ { U+0024, U+005F }) ∩ Identifier_Status=Allowed
    ID_Continue := XID_Continue ∩ Identifier_Status=Allowed

U+0024 DOLLAR SIGN and U+005F LOW LINE are added explicitly because neither is
in XID_Start; OpenSCAD needs the former for special variables ($fn, $fa, ...)
and the latter for compatibility with the previous ASCII-only syntax.

Only code points outside ASCII are emitted. The lexer handles ASCII with its own
character classes, so putting it in the tables as well would only cost lookups.

Usage:

    scripts/generate-unicode-identifier-tables.py \\
        --derived-core-properties DerivedCoreProperties.txt \\
        --identifier-status IdentifierStatus.txt \\
        --output src/core/UnicodeIdentifierTables.h

DerivedCoreProperties.txt is part of the UCD (https://www.unicode.org/Public/),
IdentifierStatus.txt is part of the UTS #39 data files
(https://www.unicode.org/Public/security/). Both files must be taken from the
same Unicode version.
"""

import argparse
import re
import sys

DOLLAR_SIGN = 0x0024
LOW_LINE = 0x005F

VERSION_PATTERN = re.compile(r'^#\s*Version:\s*(\S+)', re.MULTILINE)
DERIVED_VERSION_PATTERN = re.compile(r'^#\s*\S+-(\d+\.\d+\.\d+)\.txt', re.MULTILINE)


def parse_range(field):
    """Turn a '0041' or '0041..005A' code point field into an inclusive pair."""
    if '..' in field:
        first, last = field.split('..')
        return int(first, 16), int(last, 16)
    value = int(field, 16)
    return value, value


def read_property(path, wanted):
    """Collect all code points carrying the given property from a UCD data file."""
    code_points = set()
    with open(path, encoding='utf-8') as data:
        for line in data:
            line = line.split('#')[0].strip()
            if not line:
                continue
            fields = [field.strip() for field in line.split(';')]
            if len(fields) < 2 or fields[1] != wanted:
                continue
            first, last = parse_range(fields[0])
            code_points.update(range(first, last + 1))
    if not code_points:
        raise SystemExit("error: no '%s' entries found in %s" % (wanted, path))
    return code_points


def read_version(path, pattern):
    with open(path, encoding='utf-8') as data:
        header = data.read(4096)
    match = pattern.search(header)
    return match.group(1) if match else None


def to_ranges(code_points):
    """Turn a set of code points into a sorted list of inclusive ranges."""
    ranges = []
    first = None
    previous = None
    for code_point in sorted(code_points):
        if first is None:
            first = code_point
        elif code_point != previous + 1:
            ranges.append((first, previous))
            first = code_point
        previous = code_point
    if first is not None:
        ranges.append((first, previous))
    return ranges


def format_table(name, ranges):
    # One range per line keeps the diff readable when the tables are
    # regenerated for a new Unicode version, so clang-format is turned off
    # rather than letting it pack several ranges onto one line.
    lines = ['// clang-format off', 'inline constexpr Range %s[] = {' % name]
    for first, last in ranges:
        lines.append('  {0x%04X, 0x%04X},' % (first, last))
    lines.append('};')
    lines.append('// clang-format on')
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--derived-core-properties', required=True,
                        help='path to DerivedCoreProperties.txt')
    parser.add_argument('--identifier-status', required=True,
                        help='path to IdentifierStatus.txt')
    parser.add_argument('--output', required=True,
                        help='path of the header to write')
    args = parser.parse_args()

    xid_start = read_property(args.derived_core_properties, 'XID_Start')
    xid_continue = read_property(args.derived_core_properties, 'XID_Continue')
    allowed = read_property(args.identifier_status, 'Allowed')

    ucd_version = read_version(args.derived_core_properties, DERIVED_VERSION_PATTERN)
    security_version = read_version(args.identifier_status, VERSION_PATTERN)
    if ucd_version is None or security_version is None:
        raise SystemExit('error: cannot determine the Unicode version of the input files')
    if ucd_version != security_version:
        raise SystemExit('error: version mismatch, UCD is %s but the security data is %s'
                         % (ucd_version, security_version))

    id_start = ((xid_start & allowed) | {DOLLAR_SIGN, LOW_LINE})
    id_continue = xid_continue & allowed

    # The profile must be closed under NFC, otherwise normalising an accepted
    # identifier could turn it into a rejected one. This holds for XID by
    # construction and survives the UTS #39 intersection, but the tables are
    # regenerated rarely enough that it is worth paying for the check.
    import unicodedata
    for code_point in sorted(id_start | id_continue):
        composed = unicodedata.normalize('NFC', chr(code_point))
        allowed_here = id_start if code_point in id_start else id_continue
        if ord(composed[0]) not in allowed_here:
            raise SystemExit('error: U+%04X leaves the profile under NFC' % code_point)
        if any(ord(char) not in id_continue for char in composed[1:]):
            raise SystemExit('error: U+%04X leaves the profile under NFC' % code_point)

    start_ranges = to_ranges({c for c in id_start if c > 0x7F})
    continue_ranges = to_ranges({c for c in id_continue if c > 0x7F})

    header = '''/*
 * Identifier character tables, generated by
 * scripts/generate-unicode-identifier-tables.py -- do not edit.
 *
 * Generated from the Unicode Character Database %s (DerivedCoreProperties.txt)
 * and the UTS #39 security data of the same version (IdentifierStatus.txt).
 *
 * The profile is the default identifier syntax of UAX #31 restricted by the
 * general security profile of UTS #39, with U+0024 DOLLAR SIGN and U+005F
 * LOW LINE added to the set of starter characters:
 *
 *   ID_Start    := (XID_Start + { $, _ }) & Identifier_Status=Allowed
 *   ID_Continue := XID_Continue & Identifier_Status=Allowed
 *
 * Only non-ASCII code points are tabulated; ASCII is handled by the lexer.
 */
#pragma once

#include <cstdint>

namespace UnicodeIdentifierTables {

inline constexpr char UNICODE_VERSION[] = "%s";

/** An inclusive range of code points. */
struct Range {
  uint32_t first;
  uint32_t last;
};

/** Non-ASCII code points that may start an identifier. */
%s

/** Non-ASCII code points that may continue an identifier. */
%s

}  // namespace UnicodeIdentifierTables
''' % (ucd_version, ucd_version,
       format_table('ID_START', start_ranges),
       format_table('ID_CONTINUE', continue_ranges))

    with open(args.output, 'w', encoding='utf-8') as output:
        output.write(header)

    print('%s: Unicode %s, %d start ranges, %d continue ranges, %d bytes of tables'
          % (args.output, ucd_version, len(start_ranges), len(continue_ranges),
             (len(start_ranges) + len(continue_ranges)) * 8), file=sys.stderr)


if __name__ == '__main__':
    main()
