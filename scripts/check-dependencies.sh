# Parse the minimum versions of dependencies from README.md, and compare
# with what is found on the system. Print results.
#
# usage
#  check-dependencies.sh                # check version of all dependencies
#  check-dependencies.sh debug          # debug this script
#
# design
#  Detection is done through stages and fallbacks in case of failure.
#
#    1st stage, search by parsing header files and/or binary output
#    2nd stage, search with pkg-config
#    3rd stage, search by parsing output of package systems like dpkg or yum
#
#  Goal is portability and lack of complicated regex.
#  Code style is 'pretend its python'. functions return strings under
#  the $function_name_result variable. tmp variables are
#  funcname_abbreviated_tmp. Local vars are not used for portability.
#
# todo
#  if /usr/ and /usr/local/ on linux both hit, throw an error
#  fallback- pkgconfig --exists, then --modversion
#  fallback2 - pkg manager
#  - print location found, how found???
#
DEBUG=

debug()
{
  if [ $DEBUG ]; then echo check-dependencies.sh: $* ; fi
}


eigen_sysver()
{
  debug eigen
  eigpath=
  eig3path=$1/include/eigen3/Eigen/src/Core/util/Macros.h
  eig2path=$1/include/eigen2/Eigen/src/Core/util/Macros.h
  if [ -e $eig3path ]; then eigpath=$eig3path; fi
  if [ -e $eig2path ]; then eigpath=$eig2path; fi
  debug $eig2path
  if [ ! $eigpath ]; then return; fi
  eswrld=`grep "define  *EIGEN_WORLD_VERSION  *[0-9]*" $eigpath | awk '{print $3}'`
  esmaj=`grep "define  *EIGEN_MAJOR_VERSION  *[0-9]*" $eigpath | awk '{print $3}'`
  esmin=`grep "define  *EIGEN_MINOR_VERSION  *[0-9]*" $eigpath | awk '{print $3}'`
  eigen_sysver_result="$eswrld.$esmaj.$esmin"
}

opencsg_sysver()
{
  debug opencsg_sysver
  if [ ! -e $1/include/opencsg.h ]; then return; fi
  ocsgver=`grep "define  *OPENCSG_VERSION_STRING *[0-9x]*" $1/include/opencsg.h`
  ocsgver=`echo $ocsgver | awk '{print $4}' | sed s/'"'//g | sed s/[^1-9.]//g`
  opencsg_sysver_result=$ocsgver
}

cgal_sysver()
{
  cgalpath=$1/include/CGAL/version.h
  if [ ! -e $cgalpath ]; then return; fi
  cgal_sysver_result=`grep "define  *CGAL_VERSION  *[0-9.]*" $cgalpath | awk '{print $3}'`
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
  # on some systems you have VERSION in gmp-$arch.h not gmp.h. use gmp*.h
  if [ ! -e $1/include ]; then return; fi
  gmppaths=`ls $1/include | grep ^gmp`
  if [ ! "$gmppaths" ]; then return; fi
  for gmpfile in $gmppaths; do
    gmppath=$1/include/$gmpfile
    if [ "`grep __GNU_MP_VERSION $gmppath`" ]; then
      gmpmaj=`grep "define  *__GNU_MP_VERSION  *[0-9]*" $gmppath | awk '{print $3}'`
      gmpmin=`grep "define  *__GNU_MP_VERSION_MINOR  *[0-9]*" $gmppath | awk '{print $3}'`
      gmppat=`grep "define  *__GNU_MP_VERSION_PATCHLEVEL  *[0-9]*" $gmppath | awk '{print $3}'`
    fi
  done
  gmp_sysver_result="$gmpmaj.$gmpmin.$gmppat"
}

qt4_sysver()
{
  qt4path=$1/include/qt4/QtCore/qglobal.h
  if [ ! -e $qt4path ]; then
    qt4path=$1/include/QtCore/qglobal.h
  fi
  if [ ! -e $qt4path ]; then
    # netbsd
    qt4path=$1/qt4/include/QtCore/qglobal.h 
  fi
  if [ ! -e $qt4path ]; then return; fi
  qt4ver=`grep 'define  *QT_VERSION_STR  *' $qt4path | awk '{print $3}'`
  qt4ver=`echo $qt4ver | sed s/'"'//g`
  qt4_sysver_result=$qt4ver
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
  if [ ! -x $1/bin/bison ]; then return ; fi
  bison_sysver_result=`$1/bin/bison --version | grep bison | sed s/"[^0-9.]"/" "/g`
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

set_default_package_map()
{
  glew=glew
  boost=boost
  eigen=eigen3
  imagemagick=imagemagick
  make=make
  python=python
  opencsg=opencsg
  cgal=cgal
  bison=bison
  gmp=gmp
  mpfr=mpfr
  bash=bash
  flex=flex
  gcc=gcc
  cmake=cmake
  curl=curl
  git=git
  qt4=qt4
}


apt_pkg_search()
{

  if [ ! "`command -v dpkg`" ]; then
    # can't handle systems that use apt-get for RPMs (alt linux)
    debug command dpkg not found. cannot search packages.
    return
  fi

  debug apt_pkg_search $*
  apt_pkg_search_result=
  pkgname=$1
  dps_ver=

  # translate pkgname to apt packagename
  set_default_package_map
  for pn in cgal boost mpfr opencsg qt4; do eval $pn="lib"$pn"-dev" ; done

  # handle multiple version names of same package (ubuntu, debian, etc)
  if [ $pkgname = glew ]; then
    glewtest=`apt-cache search libglew-dev`
    if [ "`echo $glewtest | grep glew1.6-dev`" ]; then glew=libglew1.6-dev;
    elif [ "`echo $glewtest | grep glew1.5-dev`" ]; then glew=libglew1.5-dev;
    elif [ "`echo $glewtest | grep glew-dev`" ]; then glew=libglew-dev; fi
  elif [ $pkgname = gmp ]; then
    if [ "`apt-cache search libgmp3-dev`" ]; then gmp=libgmp3-dev ;fi
    if [ "`apt-cache search libgmp-dev`" ]; then gmp=libgmp-dev ;fi
  fi

  debpkgname=`eval echo "$"$pkgname`

  if [ ! $debpkgname ]; then
    debug "unknown package" $pkgname; return;
  fi

  debug $pkgname ".deb name:" $debpkgname

  # examples of apt version strings
  # cgal 4.0-4   gmp 2:5.0.5+dfsg  bison 1:2.5.dfsg-2.1 cmake 2.8.9~rc1

  if [ $pkgname = eigen ]; then
    aps_null=`dpkg --status libeigen3-dev 2>&1`
    if [ $? = 0 ]; then
      debpkgname=libeigen3-dev
    else
      debpkgname=libeigen2-dev
    fi
  fi

  debug "test dpkg on $debpkgname"
  testdpkg=`dpkg --status $debpkgname 2>&1`
  if [ "$testdpkg" ]; then
    #debug test dpkg: $testdpkg
    if [ "`echo $testdpkg | grep -i version`" ]; then
      dps_ver=`dpkg --status $debpkgname | grep -i ^version: | awk ' { print $2 }'`
      debug version line from dpkg: $dps_ver
      dps_ver=`echo $dps_ver | tail -1 | sed s/"[-~].*"// | sed s/".*:"// | sed s/".dfsg*"//`
      debug version: $dps_ver
    else
      debug couldnt find version string after dpkg --status $debpkgname
    fi
  else
    debug testdpkg failed on $debpkgname
  fi

  # Available to be system
  #dps_ver=
  #debug "test apt-cache on $debpkgname"
  # apt-cache show is flaky on older apt. dont run unless search is OK
  #test_aptcache=`apt-cache search $debpkgname`
  #if [ "$test_aptcache" ]; then
  #  test_aptcache=`apt-cache show $debpkgname`
  #  if [ ! "`echo $test_aptcache | grep -i no.packages`" ]; then
  #    ver=`apt-cache show $debpkgname | grep ^Version: | awk ' { print $2 }'`
  #    ver=`echo $ver | tail -1 | sed s/"[-~].*"// | sed s/".*:"// | sed s/".dfsg*"//`
  #    if [ $ver ] ; then vera=$ver ; fi
  #  fi
  #fi

  apt_pkg_search_result="$dps_ver"
}

set_fedora_package_map()
{
  cgal=CGAL-devel
  eigen=eigen2-devel
  qt4=qt-devel
  imagemagick=ImageMagick
  for pn in  boost gmp mpfr glew; do eval $pn=$pn"-devel" ; done
}

yum_pkg_search()
{
  yum_pkg_search_result=
  pkgname=$1

  set_default_package_map
  set_fedora_package_map
  fedora_pkgname=`eval echo "$"$pkgname`

  debug $pkgname". fedora name:" $fedora_pkgname
  if [ ! $fedora_pkgname ]; then
    debug "unknown package" $pkgname; return;
  fi

  test_yum=`yum info $fedora_pkgname 2>&1`
  if [ "$test_yum" ]; then
    debug test_yum: $test_yum
    ydvver=`yum info $fedora_pkgname 2>&1 | grep ^Version | awk '{print $3}' `
    if [ $ydvver ]; then ydvver=$ydvver ; fi
  else
    debug test_yum failed on $pkgname
  fi
  yum_pkg_search_result="$ydvver"
}


pkg_search()
{
  debug pkg_search $*
  pkg_search_result=

  if [ "`command -v apt-get`" ]; then
    apt_pkg_search $*
    pkg_search_result=$apt_pkg_search_result
  elif [ "`command -v yum`" ]; then
    yum_pkg_search $*
    pkg_search_result=$yum_pkg_search_result
  else
    debug unknown system type. cannot search packages.
  fi
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
  if [ -e README.md ]; then READFILE=README.md; fi
  if [ -e ../README.md ]; then READFILE=../README.md; fi
  if [ ! $READFILE ]; then
    if [ "`command -v dirname`" ]; then
      READFILE=`dirname $0`/../README.md
    fi
  fi
  if [ ! $READFILE ]; then echo "cannot find README.md"; exit 1; fi
  debug get_minversion_from_readme $*
  if [ ! $1 ]; then return; fi
  depname=$1
  local grv_tmp=
  debug $depname
  # example-->     * [CGAL (3.6 - 3.9)] (www.cgal.org)  becomes 3.6
  # steps: eliminate *, find left (, find -, make 'x' into 0, delete junk
  grv_tmp=`grep -i ".$depname.*([0-9]" $READFILE | sed s/"*"//`
  debug $grv_tmp
  grv_tmp=`echo $grv_tmp | awk -F"(" '{print $2}'`
  debug $grv_tmp
  grv_tmp=`echo $grv_tmp | awk -F"-" '{print $1}'`
  debug $grv_tmp
  grv_tmp=`echo $grv_tmp | sed s/"x"/"0"/g`
  debug $grv_tmp
  grv_tmp=`echo $grv_tmp | sed s/"[^0-9.]"//g`
  debug $grv_tmp
  get_minversion_from_readme_result=$grv_tmp
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
  # change x.y.z.p into x0y0z0p for purposes of comparison with -lt or -gt
  # it will work as long as the resulting int is less than 2.147 billion
  # and y z and p are less than 99
  # 1.2.3.4 into 1020304
  # 1.11.0.12 into 1110012
  # 2011.2.3 into 20110020300
  # the resulting integer can be simply compared using -lt or -gt
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
  if [ $1 ]; then pp_dep=$1; fi
  if [ $pp_dep = "title" ]; then
    echo -e $pp_title | awk $pp_format
    return ;
  fi

  if [ $2 ]; then pp_minver=$2; else pp_minver="unknown"; fi
  if [ $3 ]; then pp_foundver=$3; else pp_foundver="unknown"; fi
  if [ $4 ]; then pp_compared=$4; else pp_compared="NotOK"; fi

  if [ $pp_compared = "NotOK" ]; then
    pp_foundcolor=$purple;
    pp_cmpcolor=$purple;
  else
    pp_foundcolor=$gray;
    pp_cmpcolor=$green;
  fi
  echo -e $cyan $pp_dep $gray $pp_minver $pp_foundcolor $pp_foundver $pp_cmpcolor $pp_compared | awk $pp_format
  pp_dep=
  pp_minver=
  pp_foundver=
  pp_compared=
}




find_installed_version()
{
  debug find_installed_version $*
  find_installed_version_result=unknown
  fsv_tmp=
  dep=$1

  # try to find/parse headers and/or binary output
  if [ ! $fsv_tmp ]; then
    for syspath in "/opt" "/usr/pkg" "/usr" "/usr/local" $OPENSCAD_LIBRARIES; do
      if [ -e $syspath ]; then
        debug $dep"_sysver" $syspath
        eval $dep"_sysver" $syspath
        fsv_tmp=`eval echo "$"$dep"_sysver_result"`
      fi
    done
  fi

  # use pkg-config to search
  if [ ! $fsv_tmp ]; then
    if [ "`command -v pkg-config`" ]; then
      debug plain search failed. trying pkg_config...
      pkg_config_search $dep
      fsv_tmp=$pkg_config_search_result
    fi
  fi

  # use the package system to search
  if [ ! $fsv_tmp ]; then
    debug plain + pkg_config search both failed... trying package search
    pkg_search $dep
    fsv_tmp=$pkg_search_result
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
    header_list="opencsg.h CGAL boost GL/glew.h"
    liblist="libboost libopencsg libCGAL libglew"
    for i in $header_list $liblist; do
      if [ -e /usr/local/include/$i ]; then
        echo "Warning: you have a copy of "$i" under /usr/local/include"
        warnon=1
      fi
      if [ -e /usr/local/lib/$i.so ]; then
        echo "Warning: you have a copy of "$i" under /usr/local/lib"
        warnon=1
      fi
    done
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
}


checkargs()
{
  for i in $*; do
    if [ $i = "debug" ]; then DEBUG=1 ; fi
  done
}

main()
{
  deps="qt4 cgal gmp mpfr boost opencsg glew eigen gcc"
  deps="$deps bison flex make"
  #deps="$deps curl git" # not technically necessary for build
  #deps="$deps python cmake" # only needed for tests
  #deps="$deps imagemagick" # needs work, only needed for tests
  #deps="eigen glew opencsg" # debugging
  pretty_print title
  for dep in $deps; do
    debug "processing $dep"
    find_installed_version $dep
    dep_sysver=$find_installed_version_result
    find_min_version $dep
    dep_minver=$find_min_version_result
    compare_version $dep_minver $dep_sysver
    dep_compare=$compare_version_result
  	pretty_print $dep $dep_minver $dep_sysver $dep_compare
  done
  check_old_local
  check_misc
}

checkargs $*
main
exit 0

