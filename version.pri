# get VERSION from system date

isEmpty(VERSION) {
  win32*:!mingw-cross-env:!mingw-cross-env-shared {
    # 
    # Windows XP date command only has one argument, /t
    # and it can print the date in various localized formats. 
    # This code will detect MM/DD/YYYY, YYYY/MM/DD, and DD/MM/YYYY
    #
    SYSDATE = $$system(date /t)
    SYSDATE = $$replace(SYSDATE,"/",".")
    SYSDATE ~= s/[A-Za-z]*// # remove name of day
    DATE_SPLIT=$$split(SYSDATE, ".")
    DATE_X=$$member(DATE_SPLIT, 0)
    DATE_Y=$$member(DATE_SPLIT, 1)
    DATE_Z=$$member(DATE_SPLIT, 2)
    TEST1=$$find(DATE_X, [0-9]{4} )
    TEST2=$$find(DATE_Z, [0-9]{4} )

    QDATE = $$_DATE_
    QDATE_SPLIT = $$split(QDATE)
    QDAY = $$num_add($$member(QDATE_SPLIT,2))
    
    !isEmpty(TEST1) { 
      equals( QDAY, $$num_add($$DATE_Z) ) {
        # message("Assuming YYYY/MM/DD format")
        VERSION_YEAR = $$DATE_X 
        VERSION_MONTH = $$DATE_Y
        VERSION_DAY = $$DATE_Z
      } 
    } else {
      !isEmpty(TEST2) { 
        equals( QDAY, $$num_add($$DATE_X) ) {
          # message("Assuming DD/MM/YYYY format" $$DATE_X $$DATE_Y $$DATE_Z )
          VERSION_DAY = $$DATE_X
          VERSION_MONTH = $$DATE_Y
          VERSION_YEAR = $$DATE_Z
        } else {
          # message("Assuming MM/DD/YYYY format" $$DATE_X $$DATE_Y $$DATE_Z )
          VERSION_MONTH = $$DATE_X
          VERSION_DAY = $$DATE_Y
          VERSION_YEAR = $$DATE_Z
        }
      } else {
        # test1 and test2 both empty
        error("Couldn't parse Windows date. please run 'qmake VERSION=YYYY.MM.DD' with todays date")
      }
    } # isEmpty(TEST1)
    VERSION = $$VERSION_YEAR"."$$VERSION_MONTH"."$$VERSION_DAY
    # message("YMD Version:" $$VERSION)
  } else { 
    # Unix/Mac 
    VERSION = $$system(date "+%Y.%m.%d")
  }
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
