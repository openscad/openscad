# Parse the minimum versions of dependencies from README.md, and compare
# with what is found on the system. Print results.
#
# usage
#  check-dependencies.sh                # check version of all dependencies
#  check-dependencies.sh debug          # debug this script
#
# output
#   a table displaying the minimum version from README, the found version,
#   and whether it is OK or not.
#
# design
#   stage 1. search by parsing header files and/or binary output (_sysver)
#   stage 2. search with pkg-config
#
#  Code style is portability and simplicity. Plain sed, awk, grep, sh.
#  Functions return strings under $function_name_result variable.
#  tmp variables are named funcname_abbreviated_tmp.
#  Local vars are not used.
#
# todo
#  testing of non-bash shells
#  if /usr/ and /usr/local/ on linux both hit, throw a warning
#  print location found, how found???
#  look at pkgconfig --exists & --modversion
#  deal with deps like GLEW that don't have proper version strings?
#

DEBUG=

debug()
{
  if [ $DEBUG ]; then echo check-dependencies.sh: $* ; fi
}


eigen_sysver()
{
  debug eigen
  eigpath=$1/include/eigen3/Eigen/src/Core/util/Macros.h
  debug $eigpath
  if [ ! -e $eigpath ]; then return; fi
  eswrld=`grep "define  *EIGEN_WORLD_VERSION  *[0-9]*" $eigpath | awk '{print $3}'`
  esmaj=`grep "define  *EIGEN_MAJOR_VERSION  *[0-9]*" $eigpath | awk '{print $3}'`
  esmin=`grep "define  *EIGEN_MINOR_VERSION  *[0-9]*" $eigpath | awk '{print $3}'`
  eigen_sysver_result="$eswrld.$esmaj.$esmin"
}

opencsg_sysver()
{
  debug opencsg_sysver
  if [ ! -e $1/include/opencsg.h ]; then return; fi
  ocsgver=`grep -a "define  *OPENCSG_VERSION_STRING *[0-9x]*" $1/include/opencsg.h`
  ocsgver=`echo $ocsgver | awk '{print $4}' | sed s/'"'//g | sed s/[^1-9.]//g`
  opencsg_sysver_result=$ocsgver
}

cgal_sysver()
{
  cgalpath=$1/include/CGAL/version.h
  if [ ! -e $cgalpath ]; then return; fi
  cgal_sysver_result=`grep "define  *CGAL_VERSION  *[0-9.]*" $cgalpath | awk '{print $3}'`
}

glib2_sysver()
{
  #Get architecture triplet - e.g. x86_64-linux-gnu
  glib2archtriplet=`gcc -dumpmachine 2>/dev/null`
  if [ -z "$VAR" ]; then
    if [ "`command -v dpkg-architectures`" ]; then
      glib2archtriplet=`dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null`
    fi
  fi
  glib2path=$1/lib/$glib2archtriplet/glib-2.0/include/glibconfig.h
  if [ ! -e $glib2path ]; then
    #No glib found
    #glib can be installed in /usr/lib/i386-linux-gnu/glib-2.0/ on arch i686-linux-gnu (sometimes?)
    if [ "$glib2archtriplet" = "i686-linux-gnu" ]; then
        glib2archtriplet=i386-linux-gnu
        glib2path=$1/lib/$glib2archtriplet/glib-2.0/include/glibconfig.h
    fi
  fi
  if [ ! -e $glib2path ]; then
    glib2path=$1/lib/glib-2.0/include/glibconfig.h
  fi
  if [ ! -e $glib2path ]; then
    glib2path=$1/lib64/glib-2.0/include/glibconfig.h
  fi
  if [ ! -e $glib2path ]; then
    return
  fi
  glib2major=`grep "define  *GLIB_MAJOR_VERSION  *[0-9.]*" $glib2path | awk '{print $3}'`
  glib2minor=`grep "define  *GLIB_MINOR_VERSION  *[0-9.]*" $glib2path | awk '{print $3}'`
  glib2micro=`grep "define  *GLIB_MICRO_VERSION  *[0-9.]*" $glib2path | awk '{print $3}'`
  glib2_sysver_result="${glib2major}.${glib2minor}.${glib2micro}"
}

fontconfig_sysver()
{
  fcpath=$1/include/fontconfig/fontconfig.h
  if [ ! -e $fcpath ]; then return; fi
  fcmajor=`grep "define *FC_MAJOR.*[0-9.]*" $fcpath | awk '{print $3}'`
  fcminor=`grep "define *FC_MINOR.*[0-9.]*" $fcpath | awk '{print $3}'`
  fcrevison=`grep "define *FC_REVISION.*[0-9.]*" $fcpath | awk '{print $3}'`
  fontconfig_sysver_result="${fcmajor}.${fcminor}.${fcrevision}"
}

freetype2_sysver()
{
  freetype2path=$1/include/freetype2/freetype/freetype.h
  if [ ! -e $freetype2path ]; then return; fi
  ftmajor=`grep "define  *FREETYPE_MAJOR  *[0-9.]*" $freetype2path | awk '{print $3}'`
  ftminor=`grep "define  *FREETYPE_MINOR  *[0-9.]*" $freetype2path | awk '{print $3}'`
  ftpatch=`grep "define  *FREETYPE_PATCH  *[0-9.]*" $freetype2path | awk '{print $3}'`
  freetype2_sysver_result="${ftmajor}.${ftminor}.${ftpatch}"
}

harfbuzz_sysver()
{
  harfbuzzpath=$1/include/harfbuzz/hb-version.h
  if [ ! -e $harfbuzzpath ]; then return; fi
  hbmajor=`grep "define  *HB_VERSION_MAJOR  *[0-9.]*" $harfbuzzpath | awk '{print $3}'`
  hbminor=`grep "define  *HB_VERSION_MINOR  *[0-9.]*" $harfbuzzpath | awk '{print $3}'`
  hbmicro=`grep "define  *HB_VERSION_MICRO  *[0-9.]*" $harfbuzzpath | awk '{print $3}'`
  harfbuzz_sysver_result="${hbmajor}.${hbminor}.${hbmicro}"
}

boost_sysver()
{
  boostpath=$1/include/boost/version.hpp
  if [ ! -e $boostpath ]; then return; fi
  bsver=`grep 'define  *BOOST_LIB_VERSION *[0-9_"]*' $boostpath | awk '{print $3}'`
  bsver=`echo $bsver | sed s/'"'//g | sed s/'_'/'.'/g`
  boost_sysver_result=$bsver
}

mpfr_sysver()
{
  mpfrpath=$1/include/mpfr.h
  if [ ! -e $mpfrpath ]; then return; fi
  mpfrsver=`grep 'define  *MPFR_VERSION_STRING  *' $mpfrpath | awk '{print $3}'`
  mpfrsver=`echo $mpfrsver | sed s/"-.*"// | sed s/'"'//g`
  mpfr_sysver_result=$mpfrsver
}

gmp_sysver()
{
  gmppaths="`find $1/include -name 'gmp.h' -o -name 'gmp-*.h' 2>/dev/null`"
  if [ ! "$gmppaths" ]; then
    debug "gmp_sysver no gmp.h beneath $1"
    return
  fi
  for gmpfile in $gmppaths; do
    if [ "`grep __GNU_MP_VERSION $gmpfile`" ]; then
      gmpmaj=`grep "define  *__GNU_MP_VERSION  *[0-9]*" $gmpfile | awk '{print $3}'`
      gmpmin=`grep "define  *__GNU_MP_VERSION_MINOR  *[0-9]*" $gmpfile | awk '{print $3}'`
      gmppat=`grep "define  *__GNU_MP_VERSION_PATCHLEVEL  *[0-9]*" $gmpfile | awk '{print $3}'`
    fi
  done
  gmp_sysver_result="$gmpmaj.$gmpmin.$gmppat"
}

qt_sysver()
{
  if [ "`command -v qtchooser`" ]; then
    qtver=`qtchooser -run-tool=qmake -qt=5 -v 2>&1`
    if [ $? -eq 0 ] ; then
      export QT_SELECT=5
    else
      qtver=`qtchooser -run-tool=qmake -qt=4 -v 2>&1`
      if [ $? -eq 0 ] ; then
	export QT_SELECT=4
      fi
    fi
    qtver=`echo "$qtver" | grep "Using Qt version" | awk '{print $4}'`
  else
    export QT_SELECT=5
    qtpath=$1/include/qt5/QtCore
    if [ ! -e $qtpath ]; then
      qtpath=$1/include/i686-linux-gnu/qt5/QtCore
    fi
    if [ ! -e $qtpath ]; then
      qtpath=$1/include/x86_64-linux-gnu/qt5/QtCore
    fi
    if [ ! -e $qtpath ]; then
      export QT_SELECT=4
      qtpath=$1/include/qt4/QtCore/
    fi
    if [ ! -e $qtpath ]; then
      qtpath=$1/include/QtCore
    fi
    if [ ! -e $qtpath ]; then
      # netbsd
      qtpath=$1/qt4/include/QtCore
    fi
  fi
  if [ -z "$qtver" ]; then
    if [ ! -e "$qtpath" ]; then
      unset QT_SELECT
      return
    fi
    qtver=`grep 'define  *QT_VERSION_STR  *' "$qtpath"/qglobal.h`
    # fix for Qt 5.7
    if [ -z "$qtver" ]; then
	  qtver=`grep 'define  *QT_VERSION_STR  *' "$qtpath"/qconfig.h`
    fi
    
    qtver=`echo $qtver | awk '{print $3}' | sed s/'"'//g`
  fi
  qt_sysver_result=$qtver
}

qscintilla2_sysver()
{
  # expecting the QT_SELECT already set in case we found qtchooser
  if qmake -v >/dev/null 2>&1 ; then
    QMAKE=qmake
  elif [ "`command -v qmake-qt4`" ]; then
    QMAKE=qmake-qt4
  fi
  debug using qmake: $QMAKE

  qtincdir="`$QMAKE -query QT_INSTALL_HEADERS`"
  qscipath="$qtincdir/Qsci/qsciglobal.h"
  debug using qtincdir: $qtincdir
  debug using qscipath: $qscipath
  if [ ! -e $qscipath ]; then
    debug qscipath doesnt exist. giving up on version.
    return
  fi

  qsciver=`grep define.*QSCINTILLA_VERSION_STR "$qscipath" | awk '{print $3}'`
  qsciver=`echo $qsciver | sed s/'"'//g`
  qscintilla2_sysver_result="$qsciver"
}

glew_sysver()
{
  glewh=$1/include/GL/glew.h
  if [ -e $glewh ]; then
    # glew has no traditional version number in it's headers
    # so we either test for what we need and 'guess', or assign it to 0.0
    # the resulting number is a 'lower bound', not exactly what is installed
    if [ "`grep __GLEW_VERSION_4_2 $glewh`" ]; then
      glew_sysver_result=1.7.0
    fi
    if [ ! $glew_sysver_result ]; then
      if [ "`grep __GLEW_ARB_occlusion_query2 $glewh`" ]; then
        glew_sysver_result=1.5.4
      fi
    fi
    if [ ! $glew_sysver_result ]; then
      glew_sysver_result=0.0
    fi
  fi
}

imagemagick_sysver()
{
  if [ ! -x $1/bin/convert ]; then return; fi
  imver=`$1/bin/convert --version | grep -i version`
  imagemagick_sysver_result=`echo $imver | sed s/"[^0-9. ]"/" "/g | awk '{print $1}'`
}

flex_sysver()
{
  flexbin=$1/bin/flex
  if [ -x $1/bin/gflex ]; then flexbin=$1/bin/gflex; fi # openbsd
  if [ ! -x $flexbin ]; then return ; fi
  flex_sysver_result=`$flexbin --version | sed s/"[^0-9.]"/" "/g`
}

bison_sysver()
{
  # bison (GNU Bison) 2.7.12-4996
  if [ ! -x $1/bin/bison ]; then return ; fi
  bison_sver=`$1/bin/bison --version | grep bison`
  debug bison_sver1: $bison_sver
  bison_sver=`echo $bison_sver | awk -F ")" ' { print $2 } '`
  debug bison_sver2: $bison_sver
  bison_sver=`echo $bison_sver | awk -F "-" ' { print $1 } '`
  debug bison_sver3: $bison_sver
  bison_sysver_result=$bison_sver
}

gcc_sysver()
{
  bingcc=$1/bin/g++
  if [ ! -x $1/bin/g++ ]; then
    if [ "`command -v g++`" ]; then # fallback to $PATH
      bingcc=g++;
    fi
  fi
  debug using bingcc: $bingcc
  if [ ! -x $bingcc ]; then return; fi
  if [ ! "`$bingcc --version`" ]; then return; fi
  gccver=`$bingcc --version| grep -i g++ | awk -F "(" ' { print $2 } '`
  debug g++ output1: $gccver
  gccver=`echo $gccver | awk -F ")" ' { print $2 } '`
  debug g++ output2: $gccver
  gccver=`echo $gccver | awk ' { print $1 } '`
  debug g++ output3: $gccver
  gcc_sysver_result=$gccver
}

git_sysver()
{
  if [ ! -x $1/bin/git ]; then return ; fi
  git_sysver_result=`$1/bin/git --version | grep git | sed s/"[^0-9.]"/" "/g`
}

curl_sysver()
{
  if [ ! -x $1/bin/curl ]; then return; fi
  curl_sysver_result=`$1/bin/curl --version | grep curl | sed s/"[^0-9. ]"/" "/g | awk '{print $1}'`
}

cmake_sysver()
{
  if [ ! -x $1/bin/cmake ]; then return ; fi
  cmake_sysver_result=`$1/bin/cmake --version | grep cmake | sed s/"[^0-9.]"/" "/g | awk '{ print $1 }'`
}

make_sysver()
{
  make_sysver_tmp=
  binmake=$1/bin/make
  if [ -x $1/bin/gmake ]; then binmake=$1/bin/gmake ;fi
  if [ ! -x $binmake ]; then return ;fi
  make_sysver_tmp=`$binmake --version 2>&1`

  debug finding gnu make: raw make response: $make_sysver_tmp
  if [ ! "`echo $make_sysver_tmp | grep -i gnu`" ]; then
    return;
  fi

  make_sysver_tmp=`$binmake --version 2>&1 | grep -i 'gnu make' | sed s/"[^0-9.]"/" "/g`
  if [ "`echo $make_sysver_tmp | grep [0-9]`" ]; then
    make_sysver_result=$make_sysver_tmp
  fi
}

bash_sysver()
{
  if [ -x /bin/bash ]; then binbash=/bin/bash ;fi
  if [ -x /usr/bin/bash ]; then binbash=/usr/bin/bash ;fi
  if [ -x $1/bin/bash ]; then binbash=$1/bin/bash ;fi
  if [ ! -x $binbash ]; then return; fi
  bash_sysver_result=`$binbash --version | grep bash | sed s/"[^0-9. ]"/" "/g|awk '{print $1}'`
}

python_sysver()
{
  if [ ! -x $1/bin/python ]; then return; fi
  python_sysver_result=`$1/bin/python --version 2>&1 | awk '{print $2}'`
}

pkg_config_search()
{
  debug pkg_config_search $*
  pkg_config_search_result=
  pcstmp=
  if [ ! $1 ]; then return; fi
  pkgname=$1

  pkg-config --exists $pkgname 2>&1
  if [ $? = 0 ]; then
    pkg_config_search_result=`pkg-config --modversion $pkgname`
  else
    debug pkg_config_search failed on $*, result of run was: $pcstmp
  fi
}

get_minversion_from_readme()
{
  debug get_minversion_from_readme $*

  # Extract dependency name
  if [ ! $1 ]; then return; fi
  depname=$1

  debug $depname
  local grv_tmp=
  for READFILE in README.md ../README.md "`dirname "$0"`/../README.md"
  do
    if [ ! -e "$READFILE" ]
    then
      debug "get_minversion_from_readme $READFILE not found"
      continue
    fi
    debug "get_minversion_from_readme $READFILE found"
    grep -qi ".$depname.*([0-9]" $READFILE || continue
    grv_tmp="`grep -i ".$depname.*([0-9]" $READFILE | sed s/"*"//`"
    debug $grv_tmp
    grv_tmp="`echo $grv_tmp | awk -F"(" '{print $2}'`"
    debug $grv_tmp
    grv_tmp="`echo $grv_tmp | awk -F"-" '{print $1}'`"
    debug $grv_tmp
    grv_tmp="`echo $grv_tmp | sed s/"x"/"0"/g`"
    debug $grv_tmp
    grv_tmp="`echo $grv_tmp | sed s/"[^0-9.]"//g`"
    debug $grv_tmp
    if [ "z$grv_tmp" = "z" ]
    then
      debug "get_minversion_from_readme no result for $depname from $READFILE"
      continue
    fi
    get_minversion_from_readme_result=$grv_tmp
    return 0
  done
  if [ "z$grv_tmp" = "z" ]
  then
    debug "get_minversion_from_readme no result for $depname found anywhere"
    get_minversion_from_readme_result=""
    return 0
  fi
}

find_min_version()
{
  find_min_version_result=
  fmvtmp=
  if [ ! $1 ] ; then return; fi
  fmvdep=$1
  get_minversion_from_readme $fmvdep
  fmvtmp=$get_minversion_from_readme_result

  # items not included in README.md
  if [ $fmvdep = "git" ]; then fmvtmp=1.5 ; fi
  if [ $fmvdep = "curl" ]; then fmvtmp=6 ; fi
  if [ $fmvdep = "make" ]; then fmvtmp=3 ; fi
  if [ $fmvdep = "python" ]; then fmvtmp=2 ; fi

  find_min_version_result=$fmvtmp
}

vers_to_int()
{
  # change x.y.z.p into an integer that can be compared using -lt or -gt
  # 1.2.3.4 into 1020304
  # 1.11.0.12 into 1110012
  # 2011.2.3 into 20110020300
  # it will work as long as the resulting int is less than 2.147 billion
  # and y z and p are less than 99
  vers_to_int_result=
  if [ ! $1 ] ; then return ; fi
  vtoi_ver=$1
  vtoi_test=`echo $vtoi_ver | sed s/"[^0-9.]"//g`
  debug vers_to_int $* :: vtoi_ver: $vtoi_ver vtoi_test: $vtoi_test
  if [ ! "$vtoi_test" = "$vtoi_ver" ]; then
    debug failure in version-to-integer conversion.
    debug '"'$vtoi_ver'"' has letters, etc in it. setting to 0
    vtoi_ver="0"
  fi
  vers_to_int_result=`echo $vtoi_ver | awk -F. '{print $1*1000000+$2*10000+$3*100+$4}'`
  vtoi_ver=
  vtoi_test=
}


version_less_than_or_equal()
{
  if [ ! $1 ]; then return; fi
  if [ ! $2 ]; then return; fi
  v1=$1
  v2=$2
  vers_to_int $v1
  v1int=$vers_to_int_result
  vers_to_int $v2
  v2int=$vers_to_int_result
  debug "v1, v2, v1int, v2int" , $v1, $v2, $v1int, $v2int
  if [ $v1int -le $v2int ]; then
    debug "v1 <= v2"
    return 0
  else
    debug "v1 > v2"
    return 1
  fi
  v1=
  v2=
  v1int=
  v2int=
}

compare_version()
{
  debug compare_version $*
  compare_version_result="NotOK"
  if [ ! $1 ] ; then return; fi
  if [ ! $2 ] ; then return; fi
  cvminver=$1
  cvinstver=$2
  cvtmp=
  version_less_than_or_equal $cvminver $cvinstver
  if [ $? = 0 ]; then
    cvtmp="OK"
  else
    cvtmp="NotOK"
  fi
  compare_version_result=$cvtmp
  cvtmp=
}

pretty_print()
{
  # there are four columns, passed as $1 $2 $3 and $4
  # 1 = name of dependency
  # 2 = version found in README
  # 3 = version found on system
  # 4 = whether it is OK or not

  debug pretty_print $*

  brightred="\033[40;31m"
  red="\033[40;31m"
  brown="\033[40;33m"
  yellow="\033[40;33m"
  white="\033[40;37m"
  purple="\033[40;35m"
  green="\033[40;32m"
  cyan="\033[40;36m"
  gray="\033[40;37m"
  nocolor="\033[0m"

  ppstr="%s%-12s"
  pp_format='{printf("'$ppstr$ppstr$ppstr$ppstr$nocolor'\n",$1,$2,$3,$4,$5,$6,$7,$8)}'
  pp_title="$gray depname $gray minimum $gray found $gray OKness"
  if [ $1 ]; then pp_depname=$1; fi
  if [ $pp_depname = "title" ]; then
    echo -e $pp_title | awk $pp_format
    return ;
  fi

  if [ $2 ]; then pp_minver=$2; else pp_minver="unknown"; fi
  if [ $3 ]; then pp_foundver=$3; else pp_foundver="unknown"; fi
  if [ $4 ]; then pp_okness=$4; else pp_okness="NotOK"; fi

  if [ $pp_okness = "NotOK" ]; then
    pp_foundcolor=$purple;
    pp_cmpcolor=$purple;
  else
    pp_foundcolor=$gray;
    pp_cmpcolor=$green;
  fi
  echo -e $cyan $pp_depname $gray $pp_minver $pp_foundcolor $pp_foundver $pp_cmpcolor $pp_okness | awk $pp_format
  pp_depname=
  pp_minver=
  pp_foundver=
  pp_okness=
}

find_installed_version()
{
  debug find_installed_version $*
  find_installed_version_result=unknown
  fsv_tmp=
  depname=$1

  # try to find/parse headers and/or binary output
  # break on the first match. (change the order to change precedence)
  if [ ! $fsv_tmp ]; then
    for syspath in $OPENSCAD_LIBRARIES "/usr/local" "/opt/local" "/usr/pkg" "/usr"; do
      if [ -e $syspath ]; then
        debug $depname"_sysver" $syspath
        eval $depname"_sysver" $syspath
        fsv_tmp=`eval echo "$"$depname"_sysver_result"`
        if [ $fsv_tmp ]; then break; fi
      fi
    done
  fi

  # use pkg-config to search
  if [ ! $fsv_tmp ]; then
    if [ "`command -v pkg-config`" ]; then
      debug plain search failed. trying pkg_config...
      pkg_config_search $depname
      fsv_tmp=$pkg_config_search_result
    fi
  fi

  if [ $fsv_tmp ]; then
    find_installed_version_result=$fsv_tmp
  else
    debug all searches failed. unknown version.
  fi
}

check_old_local()
{
  warnon=
  if [ "`uname | grep -i linux`" ]; then
    header_list="opencsg.h CGAL boost GL/glew.h gmp.h mpfr.h eigen3"
    for i in $header_list; do
      if [ -e /usr/local/include/$i ]; then
        echo "Warning: you have a copy of "$i" under /usr/local/include"
        warnon=1
      fi
    done
    liblist="libboost_system libboost_system-mt libopencsg libCGAL libglew"
    for i in $liblist; do
      if [ -e /usr/local/lib/$i.so ]; then
        echo "Warning: you have a copy of "$i" under /usr/local/lib"
        warnon=1
      fi
    done
    if [ -e /usr/local/lib/pkgconfig ]; then
      echo "Warning: you have pkgconfig under /usr/local/lib"
      warnon=1
    fi
  fi
  if [ $warnon ]; then
    echo "Please verify these local copies don't conflict with the system"
  fi
}

check_misc()
{
  if [ "`uname -a|grep -i netbsd`" ]; then
    echo "NetBSD: Please manually verify the X Sets have been installed"
  fi

  if [ "`uname -a|grep -i darwin`" ]; then
	sparkle=
	libs="~/Library /Library"
    for libhome in $libs; do
		echo "$libhome/Frameworks/Sparkle.framework..."
		if [ -d $libhome/Frameworks/Sparkle.framework ]; then
			echo "Found in $libhome"
			sparkle=$libhome
			break
		fi
	done
	if [ -n "$sparkle" ]; then
		echo "OS X: Make sure Sparkle.framework is installed in your Frameworks path"
	else
		echo "OS X: Sparkle.framework found in $libhome"
	fi
  fi
}

checkargs()
{
  for i in $*; do
    if [ $i = "debug" ]; then DEBUG=1 ; fi
  done
}

main()
{
  deps="qt qscintilla2 cgal gmp mpfr boost opencsg glew eigen glib2 fontconfig freetype2 harfbuzz bison flex make"
  #deps="$deps curl git" # not technically necessary for build
  #deps="$deps python cmake imagemagick" # only needed for tests
  #deps="cgal"
  pretty_print title
  for depname in $deps; do
    debug "processing $dep"
    find_installed_version $depname
    dep_sysver=$find_installed_version_result
    find_min_version $depname
    dep_minver=$find_min_version_result
    compare_version $dep_minver $dep_sysver
    dep_compare=$compare_version_result
    pretty_print $depname "$dep_minver" "$dep_sysver" $dep_compare
  done
  check_old_local
  check_misc
}

checkargs $*
main
exit 0

