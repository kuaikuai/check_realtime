#!/bin/sh

##########################################
#  FIM Build
##########################################


HOME="${CODE_PATH}//check_realtime"
PACKAGENAME=FIM
rpmsource=/usr/src/packages/SOURCES/${PACKAGENAME}.tar.gz

cd "${HOME}"
tar zcf ${rpmsource} ./*


version=`echo ${CURRENT_VERSION}|awk -F '.' '{print $1}'`
subversion=`echo ${CURRENT_VERSION}|awk -F '.' '{print $2}'`
patchnum=`echo ${CURRENT_VERSION}|awk -F '.' '{print $3}'`
releasenum=`echo ${CURRENT_VERSION}|awk -F '.' '{print $4}'`
rpmbuild -bb "${HOME}/scripts/fim.spec" --define="_hitvversion ${version}.${subversion}.${patchnum}" --define="_hitvrelease ${releasenum}" --define="_topdir /usr/src/packages"  --define="__arch_install_post %nil" 

if [ $? -ne 0 ];then
echo "*******************************************"
echo "*******************************************"
echo "Build FIM Error!!!"
echo "*******************************************"
echo "*******************************************"
cd "${HOME}"
exit 0
fi

rm -f ${rpmsource}
		


cd "${HOME}"
echo ""
echo ""
echo "**************************************************************************"
echo "----------FIM Well Done!-----------"
echo "**************************************************************************"
echo ""
echo ""
