echo "tested on OpenSUSE 12. If this fails try 'old linux' build (see README.md)"

sudo zypper install libeigen2-devel mpfr-devel gmp-devel boost-devel \
 libqt4-devel glew-devel cmake git 

echo "now copy/paste the following to install CGAL and OpenCSG from source:"
echo "sudo BASEDIR=/usr/local ./scripts/linux-build-dependencies.sh cgal-use-sys-libs"
echo "sudo BASEDIR=/usr/local ./scripts/linux-build-dependencies.sh opencsg"
