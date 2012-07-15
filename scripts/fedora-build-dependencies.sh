echo "Tested on Fedora 17. If this fails try 'old linux' build (see README.md)"
sleep 2

# Fedora 17 has CGAL 4
#if [ "`yum list installed | grep -i cgal`" ]; then
#	echo "Please make sure you have removed all cgal packages and retry"
#	exit
#fi

if [ "`yum list installed | grep -i opencsg`" ]; then
	echo "Please make sure you have removed all opencsg packages and retry"
	exit
fi

sudo yum install qt-devel bison flex eigen2-devel \
 boost-devel mpfr-devel gmp-devel glew-devel CGAL-devel gcc pkgconfig git

#echo "now copy/paste the following to install CGAL and OpenCSG from source:"
#echo "sudo BASEDIR=/usr/local ./scripts/linux-build-dependencies.sh cgal-use-sys-libs"

echo
echo "Now copy/paste the following to install OpenCSG from source:"
echo
# https://bugzilla.redhat.com/show_bug.cgi?id=144967
echo "sudo echo /usr/local/lib | sudo tee -a /etc/ld.so.conf.d/local.conf"
echo "sudo ldconfig"
echo "sudo BASEDIR=/usr/local ./scripts/linux-build-dependencies.sh opencsg"
echo

