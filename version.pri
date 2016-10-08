# get VERSION from system date

isEmpty(VERSION) {
  VERSION = $$system(date "+%Y.%m.%d")
}

# Split off patch level indicator
SHORTVERSION = $$section(VERSION, "-", 0, 0)

# Split version into Year Month [Day]
VERSION_SPLIT=$$split(SHORTVERSION, ".")
VERSION_YEAR=$$member(VERSION_SPLIT, 0)
VERSION_MONTH=$$member(VERSION_SPLIT, 1)
VERSION_DAY=$$member(VERSION_SPLIT, 2)

# Fix for problem with integers with leading zeros
# being interpreted by C++ as octals. Now they're doubles.
VERSION_YEAR=$${VERSION_YEAR}.0
VERSION_MONTH=$${VERSION_MONTH}.0
VERSION_DAY=$${VERSION_DAY}.0

DEFINES += OPENSCAD_VERSION=$$VERSION OPENSCAD_SHORTVERSION=$$SHORTVERSION OPENSCAD_YEAR=$$VERSION_YEAR OPENSCAD_MONTH=$$VERSION_MONTH
!isEmpty(VERSION_DAY): DEFINES += OPENSCAD_DAY=$$VERSION_DAY

!isEmpty(OPENSCAD_COMMIT) {
  DEFINES += OPENSCAD_COMMIT=$$OPENSCAD_COMMIT
}
