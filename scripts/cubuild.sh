#!/bin/bash

build=0
docs=0
make_install=0
clean=0
cleandocs=0
meson=0
epics=0
tango=0
qml=1
random=1
websocket=0
http=0
pull=0
srcupdate=0
sudocmd=sudo

## declare operations array
declare -a operations

# this file contains some variables defined in previous executions
# of this script. For example, this is used to remember if the user
# enabled tango or epics modules.
# The file is also used by the "cumbia upgrade" bash function (/etc/profile/cumbia.sh)
# to find the location of the cumbia sources used for upgrades.
#
srcupdate_conf_f=$HOME/.config/cumbia/srcupdate.conf

# if this script has been already executed, the file will help us remember if 
# the tango and epics modules were enabled
#
if [ -r $srcupdate_conf_f ]; then
	. $srcupdate_conf_f
fi

DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi

# change into cumbia source top level folder
cd $DIR/..

topdir=$PWD

# temporary dir where one file for each module stores build date and time
tmp_build_d=$PWD/tmp-build-dir

## temporary local installation directory (to make pkg-config work)
## rm -rf $build_dir takes place after clean
##
if [ -z $build_dir ]; then
    build_dir=$PWD/build
fi

save_pkg_config_path=$PKG_CONFIG_PATH
PKG_CONFIG_PATH=$build_dir/lib/pkgconfig:$PKG_CONFIG_PATH
export PKG_CONFIG_PATH

echo "PKG_CONFIG_PATH is $PKG_CONFIG_PATH"

if [[ $@ == **help** ]]
then
        echo -e "\n\e[1;32mENVIRONMENT\e[0m\n"
        echo -e "* export install_prefix=/usr/local/cumbia-libs - specify a custom install_prefix"
        echo -e " (install_prefix is defined in $DIR/config.sh and can be edited there)"
        echo -e "* export build_dir=/my/build/dir - specify a custom build directory"
        echo -e " (build_dir is defined in $DIR/config.sh and can be edited there)"
        echo -e " libraries can be manually copied from the build_dir into the desired destination folder."
	echo -e "\n\e[1;32mOPTIONS\e[0m\n"
	echo -e " [no arguments] - build cumbia, cumbia-qtcontrols, qumbia-apps, qumbia-plugins."
	echo -e "                  No engine specific projects will be built (cumbia-tango, cumbia-epics, qumbia-tango-controls)" 
	echo -e "                  To build tango modules, add the \"tango\" argument, to build the epics modules, add \"epics\" (example 3.)"
	echo -e " pull - update sources from git"
	echo -e " docs - regenerate documentation"
	echo -e " cleandocs - remove locally generated documentation"
	echo -e " clean - execute make clean in every project folder"
	echo -e " install - execute make install from every project folder\n"
	echo -e " no-sudo - do not use \"sudo\" when calling make install\""
	echo -e " tango - include cumbia-tango and qumbia-tango-controls modules in the specified operations"
	echo -e " epics - include cumbia-epics and qumbia-epics-controls modules in the specified operations"
        echo -e " websocket - include cumbia-websocket module in the specified operations"
        echo -e " http - include cumbia-http module in the specified operations"
        echo -e " qml - build cumbia-qtcontrols QML module (default: enabled)"
	echo -e " no-tango - exclude cumbia-tango and qumbia-tango-controls modules from the specified operations"
	echo -e " no-epics - remove cumbia-epics and qumbia-epics-controls modules from the specified operations"
	echo -e " srcupdate - update cumbia sources choosing from the available tags in git (or origin/master). Please note that"
        echo -e "             the copy of the sources managed with srcupdates is not intended to be modified and committed to git."
        echo -e "             This option is normally used by the \e[0;4mcumbia upgrade\e[0m command."
	echo ""
        echo -e "\e[1;32mEXAMPLES\e[0m\n1. $0 pull clean install tango epics random - git pull sources, execute make clean, build and install cumbia, cumbia-qtcontrols, apps, plugins, the tango, EPICS and random modules"
        echo -e "2. $0 install - install the library built with command 1."
	echo -e "3. $0 tango - build cumbia, cumbia-tango, cumbia-qtcontrols, qumbia-tango-controls, qumbia-apps and qumbia-plugins"
	echo ""

	exit 0
fi

if [ ! -r $DIR/config.sh ]; then
	echo " \e[1;31merror\e[0m file $DIR/config.sh is missing"
	exit 1
fi

echo -e "\n \e[1;33minfo\e[0m: reading configuration from \"$DIR/config.sh\""
. "$DIR/projects.sh"
. "$DIR/config.sh"

echo -e -n " \e[1;33minfo\e[0m: \e[1;4minstallation prefix\e[0m is \"$install_prefix\""

if [ $prefix_from_environment -eq 1 ]; then
    echo -e " [\e[1;33m from environment variable\e[0m ]"
else
    echo -e " [\e[1;33m from scripts/config.sh settings\e[0m ]"
    echo -e " \e[1;33minfo\e[0m: You can call \e[1;32minstall_prefix=/your/desired/prefix $0\e[0m to override the setting in scripts/config.sh"
fi

if [ $build_dir_from_environment -eq 1 ]; then
    echo -e -n " \e[1;33minfo\e[0m: \e[1;4mbuild directory\e[0m is \"$build_dir\""
    echo -e " [\e[1;33m from environment variable\e[0m ]"
else
    echo -e "If you want to build the libraries into a specific \e[1;3mbuild dir\e[0m"
    echo -e "call \e[1;32mbuild_dir=/your/desired/build_dir $0\e[0m."
    echo -e "You can later manually copy the contents into the destination installation folder"
fi

echo ""

if [[ $@ == **pull** ]]
then
	pull=1
	operations+=(pull)
fi

if [[ $@ == **no-sudo** ]]
then
    $sudocmd=
fi

if [[ $@ == **build** ]]
then
        build=1
        meson=1
fi


if [[ $@ == **docs** ]]
then
    meson=1
    docs=1
	operations+=(docs)
fi

if [[ $@ == **cleandocs** ]]
then
        meson=1
        cleandocs=1
	operations+=(cleandocs)
fi

if [[ $@ == **clean** ]]
then
        meson=1
        clean=1
	operations+=(clean)
fi

if [[ $@ == **srcupdate** ]]
then
	srcupdate=1
	operations+=(srcupdate)
fi


if [[ $@ == **install** ]]
then
    if  [[ ! -r $build_dir ]] || [[ $clean -eq 1 ]] ;  then
        build=1
        meson=1
        operations+=(build)
    fi
    make_install=1
    operations+=(install)
fi

## check options compatibility
##
if [ $srcupdate -eq 1 ]  && [ "$#" -ne 1 ] ; then
	echo -e " \e[1;31merror\e[0m: \"srcupdate\" is not compatible with any other option\n"
	exit 1
fi

if [[ $@ == **tango** ]]; then
	tango=1
fi

if [[ $@ == **epics** ]]; then
	epics=1
fi

if [[ $@ == **no-tango** ]]; then
	tango=0
fi

if [[ $@ == **no-epics** ]]; then
	epics=0
fi

if [[ $@ == **qml** ]]; then
        qml=1
fi

if [[ $@ == **no-qml** ]]; then
        qml=0
fi

if [[ $@ == **random** ]]; then
        random=1
fi

if [[ $@ == **no-qml** ]]; then
        random=0
fi

if [[ $@ == **websocket** ]]; then
        websocket=1
fi

if [[ $@ == **no-websocket** ]]; then
        websocket=0
fi

if [[ $@ == **http** ]]; then
        http=1
fi

if [[ $@ == **no-http** ]]; then
        http=0
fi


if  [ "$#" == 0 ]; then
	build=1
	meson=1
fi

if [[ $tango -eq 1 ]]; then
	meson_p+=(cumbia-tango)
	qmake_p+=(qumbia-tango-controls)
fi

if [[ $epics -eq 1 ]]; then
	meson_p+=(cumbia-epics)
	qmake_p+=(qumbia-epics-controls)
fi

if [[ $websocket -eq 1 ]]; then
        qmake_p+=(cumbia-websocket)
fi

if [[ $http -eq 1 ]]; then
        qmake_p+=(cumbia-http)
fi

if [[ $qml -eq 1 ]]; then
        qmake_p+=(cumbia-qtcontrols/qml)
fi

if [[ $random -eq 1 ]]; then
        qmake_p+=(cumbia-random)
fi


if [ ${#operations[@]} -eq 0 ]; then
	operations+=(build)
	build=1
fi

## Print prompt
##
echo -e " \e[1;33minfo\e[0m:"
echo -e "  You may execute $0 --help for the list of available options"
echo -e "  Please refer to https://github.com/ELETTRA-SincrotroneTrieste/cumbia-libs README.md for installation instructions"
echo -e "  Documentation: https://elettra-sincrotronetrieste.github.io/cumbia-libs/"

echo -e "\n-----------------------------------------------------------------------"

echo ""
echo -e "\e[1;32;4mSOURCES\e[0m: \n\n * $topdir"
echo ""
echo -e "\e[1;32;4mOPERATIONS\e[0m:\n"
for x in "${operations[@]}"; do
	echo -e " * ${x}"
done

echo ""

if [[ $make_install -eq 1 ]] && [[ $build -eq 0 ]]; then
    echo -e "Will \e[1;32;4minstall\e[0m the following modules:\n"
    for f in `ls tmp-build-dir`; do
        echo -e  "\e[0;32m$f\e[0m - built on `cat tmp-build-dir/$f`"
    done

elif [[ $srcupdate -eq 0 ]]; then

	## START PRINT OPERATIONS MENU
	echo -e "The \e[1;32;4moperations\e[0m listed above will be applied to the following \e[1;32;4mMODULES\e[0m:\n"

	for x in "${meson_p[@]}" ; do
		echo -e " * ${x} [c++, meson build system]"
	done

	for x in "${qmake_p[@]}" ; do
		echo -e " * ${x} [qt,  qmake build system]"
	done

	for x in "${qmake_subdir_p[@]}"; do
		echo ""
		for sd in `ls -1 -d $DIR/../${x}/*/`; do
			echo -e " * ${sd} [qt,  qmake build system]"
		done
	done	

	if [ $tango -eq 0 ] && [ $epics -eq 0 ]; then
		echo -e " -"
		echo -e " \e[1;33minfo\e[0m"
		echo -e " \e[1;33m*\e[0m neither \e[1;31;4mtango\e[0m nor \e[1;31;4mepics\e[0m modules will be included in the \e[1;32;4mOPERATIONS\e[0m below"
		echo -e " \e[1;33m*\e[0m add \"tango\" and/or \"epics\" to enable cumbia-tango, qumbia-tango-controls"
		echo -e " \e[1;33m*\e[0m and cumbia-epics, qumbia-epics-controls respectively.\n"
	elif  [ $tango -eq 0 ]; then
		echo -e "\n \e[1;33m*\e[0m \e[1;35;4mtango\e[0m module is disabled"
	elif  [ $epics -eq 0 ]; then
		echo -e "\n \e[1;33m*\e[0m \e[1;35;4mepics\e[0m module is disabled"
	fi

	if [ $make_install -eq 1 ]; then 
		echo -e " -"
                echo -e " \e[1;32minstall\e[0m: cumbia will be installed in \"$install_prefix\""
	fi
fi ## END PRINT OPERATIONS MENU

echo -e "-----------------------------------------------------------------------"

echo ""
echo -n -e "Do you want to continue? [y|n] [y] "

read  -s -n 1  cont

if [ "$cont" != "y" ]  && [ "$cont" != "yes" ] && [ "$cont" != "" ]; then
	echo -e "\n  You may execute $0 --help"
	echo -e "  Please refer to https://github.com/ELETTRA-SincrotroneTrieste/cumbia-libs README.md for installation instructions"
	echo -e "  Documentation: https://elettra-sincrotronetrieste.github.io/cumbia-libs/"
	
	exit 1
fi

## inform the user that after the installation "cumbia upgrade" can be used to upgrade cumbia
#
if [ ! -r $srcupdate_conf_f ] && [ $make_install -eq 1 ]; then
	echo -e "\n\n \e[1;32;4mREMEMBER\e[0m:\n after the first installation, you can run \e[1;32mcumbia upgrade\e[0m to update cumbia \e[0;4mas long as\e[0m:"
	echo -e "  - this source tree is not removed from this directory (\"$topdir\")"
	echo -e "  - this source tree is not intended for development, i.e. will not be modified and committed to git"
	echo -e -n "\n Press any key to continue "
	read -s -n 1 akey
fi

if [ $pull -eq 1 ]; then
    echo -e "\e[1;32mupdating sources [git pull]...\e[0m"
	git pull
fi

if [[ $srcupdate -eq 1 ]]; then
	wdir=$PWD
	cd $topdir
	# sync tags
	
	echo -e "\e[1;34m\n*\n* UPGRADE fetching available tags...\n*\e[0m"	

	git fetch --tags

	tags=`git tag --list`
	declare -a taglist
	idx=1
	for tag in $tags ; do
		echo -e " $idx. $tag"
		((idx++))
		taglist+=($tag)
	done

        taglist+=(master)
	echo -e " $idx. ${taglist[$((idx-1))]}"
	
	echo -e -n "\ncurrent version: \e[1;32m"

	# With --abbrev set to 0, the command can be used to find the closest tagname without any suffix,
	# Otherwise, it suffixes the tag name with the number of additional commits on top of the tagged 
        # object and the abbreviated object name of the most recent commit.
	#
        git describe --tags --abbrev=0
	
	echo -e "\e[0m"
	echo -e "\nChoose a version [1, ..., $idx] or press any other key to exit"

	read  -s -n 1 choice

	## is choice an integer number??
	re='^[0-9]+$'
	if ! [[ $choice =~ $re ]] ; then
		exit 1
	fi
	
	if [ "$choice" -ge "1" ] && [ "$choice" -le "$idx" ]; then
		array_index=$((choice - 1))
		checkout_tag=${taglist[$array_index]}
		echo  -e "\e[1;34m\n*\n* UPGRADE checking out version $checkout_tag ...\n*\e[0m"
		echo -e " \e[1;33mwarning\e[0m: the srcupdate procedure is intended to be run in a \e[1;37;4mread only\e[0m source tree, where these rules apply:"
		echo -e "          1. you can switch to, build and install different cumbia releases at any time"
		echo -e "          2. you \e[0;35mshould not\e[0m modify and commit changes from here (\e[0;35m$topdir\e[0m)"
		echo ""
		echo -e -n " \e[1;32m*\e[0m Do you want to update the sources to version \e[1;32m$checkout_tag\e[0m [y|n]? [y]: "
		read  -s -n 1 reply
		
		if [ "$reply" != "y" ]  && [ "$reply" != "yes" ] && [ "$reply" != "" ]; then
		 	exit 1
		fi

		git checkout $checkout_tag

	else
		echo -e "\n \e[1;31merror\e[0m: choice \"$choice\" is outside the interval [1, $idx]\n"
		exit 1
	fi
	 
	# restore previous directory
	cd $wdir

	##
	## exit successfully after git checkout of the desired version
		
	exit 0


fi # [[ $srcupdate -eq 1 ]]

##                            
## save configuration in $HOME/.config/cumbia/srcupdate.conf
##

if [ ! -d $HOME/.config/cumbia ]; then
	mkdir -p $HOME/.config/cumbia
fi

# empty the file
> "$srcupdate_conf_f"

echo -e "\n# tango enabled" >> $srcupdate_conf_f
echo "tango=$tango" >> $srcupdate_conf_f
echo -e "\n# epics enabled" >> $srcupdate_conf_f
echo "epics=$epics" >> $srcupdate_conf_f
echo -e "\n# random module enabled" >> $srcupdate_conf_f
echo "random=$random" >> $srcupdate_conf_f

echo -e "\n# websocket module enabled" >> $srcupdate_conf_f
echo "websocket=$websocket" >> $srcupdate_conf_f

echo -e "\n# http module enabled" >> $srcupdate_conf_f
echo "http=$http" >> $srcupdate_conf_f

echo -e "\n# directory with the cumbia sources " >> $srcupdate_conf_f
echo "srcdir=$topdir" >> $srcupdate_conf_f
#
## end save configuration in $HOME/.config/cumbia/srcupdate.conf
#

if [ $cleandocs -eq 1 ]; then
	docshtmldir=$topdir/$DIR/../docs/html
	if [ -d $docshtmldir ]; then
		echo -e "\e[1;36m\n*\n* CLEAN DOCS under \"$docshtmldir\" ...\n*\e[0m"	
		cd $docshtmldir
		find . -name "html" -type d -exec rm -rf {} \;
		cd $topdir
	else
		echo -e "\e[1;36m\n*\n* CLEAN DOCS directory \"$docshtmldir\" doesn't exist.\n*\e[0m"	
	fi
	exit 0
fi

### CLEAN SECTION ###########
#
if [ $clean -eq 1 ]; then

    rm $tmp_build_d/*

# a. meson
    for x in "${meson_p[@]}" ; do
        cd $DIR/../${x}
        echo -e "\e[1;33m\n*\n* CLEAN project ${x}...\n*\e[0m"
        if [ -d builddir ]; then
            echo -e "\e[0;33m\n*\n* CLEAN project ${x}: removing \"builddir\"...\n*\e[0m"
            rm -rf builddir
        fi
        ## Back to topdir!
        cd $topdir
    done

# b. qmake
    for x in "${qmake_p[@]}"; do
        cd $DIR/../${x}
        echo -e "\e[1;33m\n*\n* CLEAN project ${x}...\n*\e[0m"
        qmake "INSTALL_ROOT=$build_dir" "prefix=$install_prefix"  && make distclean

        ## Back to topdir!
        cd $topdir
    done

# c. qmake sub projects
    for x in "${qmake_subdir_p[@]}"; do
        cd $DIR/../${x}
        for sd in `ls -1 -d */`; do
                cd ${sd}
                thisdir=${sd}
                if [[ $thisdir == */ ]]; then
                        pro_file="${thisdir::-1}.pro"
                else
                        pro_file="${thisdir}.pro"
                fi
                # a .pro file exists
                if [ -f $pro_file ]; then
                        echo -e "\e[1;33m\n*\n* CLEAN project ${sd}...\n*\e[0m"
                        qmake "INSTALL_ROOT=$build_dir" "prefix=$install_prefix"  && make distclean

                fi # -f $pro_file
                cd ..
        done
        cd ..
    done

fi  # clean -eq 1

#
# remove build_dir to clean previous build local installation
#
if  [ $make_install -eq 0 ] && [  -d $build_dir ]; then
        echo -e "\nRemoving temporary build directory: \"$build_dir\""
        rm -rf $build_dir
        if [  -d $build_dir ]; then
            rm -f $tmp_build_d/*
        fi
fi

if  [[ $make_install -eq 0 ]] &&  [[ $build -eq 0 ]] && [[ $docs -eq 0 ]]; then
    echo -e "\e[1;33m\n*\n* build, install, docs \e[0m are disabled \n\e[1;33m*\e[0m"
    exit 0
fi
#
### END CLEAN SECTION ################

if [ ! -d $tmp_build_d ]; then
    mkdir $tmp_build_d
fi

for x in "${meson_p[@]}" ; do
	cd $DIR/../${x}    

	if [ $meson -eq 1 ]; then
                if [ -d builddir ]; then
                    rm -rf builddir
                fi
                mkdir builddir
		meson builddir
	fi

        cd builddir

        ## build
	#
	if [ $build -eq 1 ]; then
		echo -e "\e[1;32m\n*\n* BUILD project ${x}...\n*\e[0m"

                ## when building need to install in build_dir
                ## pkgconfig files will be generated accordingly and will be found under build_dir/lib/pkgconfig
                ## So, build and install in build_dir

               if [ ${x} == "cumbia-epics" ]; then
                    if [[ ! -z "${EPICS_BASE}" ]] &&
                    [[ ! -z "${EPICS_HOST_ARCH}" ]]; then
                        echo  -e "\e[1;32m* \e[0mEPICS_BASE and EPICS_HOST_ARCH defined as"
                        echo  -e "\e[1;32m* \e[0mEPICS_BASE=\e[1;36m${EPICS_BASE}\e[0m"
                        echo -e  "\e[1;32m* \e[0mEPICS_HOST_ARCH=\e[1;36m${EPICS_HOST_ARCH}\e[0m"
                        echo -e "\e[1;32m* \e[0m"

                        meson configure -Depics_base=$EPICS_BASE -Depics_host_arch=$EPICS_HOST_ARCH
                        #    ninja install
                         if [ $? -ne 0 ]; then
                                 echo "meson configure output = `meson configure`"
                                 exit 1
                          fi
                    else
                        echo -e "\e[1;31m\n*\n* BUILD project ${x}: either EPICS_BASE or EPICS_HOST_ARCH (or both) are unset\n*\e[0m"
                        echo -e "\e[1;31m* BUILD project ${x}: EPICS_BASE=${EPICS_BASE}"
                        echo -e "\e[1;31m* BUILD project ${x}: EPICS_HOST_ARCH=${EPICS_HOST_ARCH}"
                        echo -e "\e[1;31m* project ${x} requires EPICS_BASE and EPICS_HOST_ARCH environment variables"
                        echo -e "\e[1;31m* - EPICS_BASE points to the directory where EPICS is installed"
                        echo -e "\e[1;31m* - EPICS_HOST_ARCH specifies the architecture epics has been built for (e.g. linux-x86_64)"
                        echo -e "\e[1;31m* \e[0m\n"
                        exit 1
                    fi
               fi
               meson configure -Dprefix=$build_dir -Dlibdir=$lib_dir -Dbuildtype=$build_type && ninja
               #    ninja install
               meson configure -Dprefix=$build_dir -Dlibdir=$lib_dir -Dbuildtype=$build_type && ninja install

               #    ninja install
                if [ $? -ne 0 ]; then
                        exit 1
                else
                    date > $tmp_build_d/${x}
                fi

	fi

	#
        ## docs
	#
	if [ $docs -eq 1 ]; then
		echo -e "\e[1;36m\n*\n* BUILD DOCS project ${x}...\n*\e[0m"
		if [ -d doc/html ]; then
			rm -rf doc/html
		fi
		ninja doc/html
		if [ $? -ne 0 ]; then
			exit 1
		fi
		docsdir=$topdir/$DIR/../docs/html/${x}
		docshtmldir=$docsdir/html
		echo -e "\e[1;36m\n*\n* COPY DOCS project ${x} into \"$docsdir\" ...\n*\e[0m"

		if [ ! -d $docsdir ]; then
			mkdir -p $docsdir
		fi	

		if [ -x $docsdir ]; then
			rm -rf $docshtmldir
			cp -a doc/html $docsdir/
			if [ $? -ne 0 ]; then
				exit 1
			fi
		else
			echo -e "\e[1;36m\n*\n* COPY DOCS \e[1;31mERROR\e[1;36m: directory \"$docsdir\" does not exist!\n*\e[0m"
			exit 1
		fi
	fi

        # Back to topdir!
	cd $topdir
done

for x in "${qmake_p[@]}"; do
	cd $DIR/../${x}

	#
	## build ###
	#
	if [ $build -eq 1 ]; then
		echo -e "\e[1;32m\n*\n* BUILD project ${x}...\n*\e[0m"
                ##
                ## build and install under build_dir
                ##
                qmake "INSTALL_ROOT=$build_dir"  "prefix=$install_prefix"  && make 
		if [ $? -ne 0 ]; then
			exit 1
		fi

                # no pkg config file to install for cumbia-qtcontrols/qml
                # moreover, it would install resources under /usr/lib64/qt5/qml/
                # which is not writable, and this stage is "build" only
                if [ "$x" != "cumbia-qtcontrols/qml" ]; then
                    make install
                    if [ $? -ne 0 ]; then
                            exit 1
                    else
                            date > $tmp_build_d/${x}
                    fi
                fi

	fi


	#
	## docs ###
	#
	if [ $docs -eq 1 ]; then
		echo -e "\e[1;36m\n*\n* BUILD DOCS project ${x}...\n*\e[0m"
		if [ -d doc ]; then
			rm -rf doc
		fi
                qmake "INSTALL_ROOT=$build_dir"  "prefix=$install_prefix"  && make doc
		if [ $? -ne 0 ]; then
			echo -e "\e[1;36m\n*\n* BUILD DOCS project ${x} has no \"doc\" target...\n*\e[0m\n"
		fi
		docsdir=$topdir/$DIR/../docs/html/${x}
		docshtmldir=$docsdir/html
		echo -e "\e[1;36m\n*\n* COPY DOCS project ${x} into \"$docsdir\" ...\n*\e[0m"
		
		if [ ! -d $docsdir ]; then
			mkdir -p $docsdir
		fi

		if [ -d $docsdir ]; then
			rm -rf $docshtmldir
			cp -a doc/html $docsdir/
			if [ $? -ne 0 ]; then
				exit 1
			fi
		else
			echo -e "\e[1;36m\n*\n* COPY DOCS \e[1;31mERROR\e[1;36m: directory \"$docsdir\" does not exist!\n*\e[0m"
			exit 1
		fi
	fi

        cd $topdir

done

savedir=$DIR

for x in "${qmake_subdir_p[@]}"; do

	cd $DIR/../${x}

#        ## with subdirs template there may be dependencies
#        ## Rely on qmake to correctly manage subdirs
#        if [ $build -eq 1 ]; then
#                echo -e "\e[1;32m\n*\n* BUILD project ${sd}...\n*\e[0m"
#                qmake "INSTALL_ROOT=$build_dir"  "prefix=$install_prefix"  && make  && make install
#                if [ $? -ne 0 ]; then
#                        exit 1
#                else
#                        date > "$tmp_build_d/${x}-->${dire_nam}"
#                fi
#        fi

	for sd in `ls -1 -d */`; do
		cd ${sd} 
		thisdir=${sd}
		if [[ $thisdir == */ ]]; then
			pro_file="${thisdir::-1}.pro"	
                        dire_nam="${thisdir::-1}"
		else
                        pro_file="${thisdir}.pro"
                        dire_nam="${thisdir}"
		fi

		if [ -f $pro_file ]; then
			#
                        if [ $build -eq 1 ]; then
                                echo -e "\e[1;32m\n*\n* BUILD project ${sd}...\n*\e[0m"
                                qmake "INSTALL_ROOT=$build_dir"  "prefix=$install_prefix"  && make  && make install
                                if [ $? -ne 0 ]; then
                                        exit 1
                                else
                                        date > "$tmp_build_d/${x}-->${dire_nam}"
                                fi
                        fi

			if [ $docs -eq 1 ]; then	
				echo -e "\e[1;36m\n*\n* BUILD DOCS project ${sd}...\n*\e[0m"
				
				if [ -d doc ]; then
					rm -rf doc
				fi
			
                                qmake "INSTALL_ROOT=$build_dir"  "prefix=$install_prefix"  && make doc
				if [ $? -ne 0 ]; then
					echo -e "\e[1;36m\n*\n* BUILD DOCS project ${sd} has no \"doc\" target...\n*\e[0m\n"
				else
					docsdir=$topdir/$DIR/../docs/html/${sd}
					docshtmldir=$docsdir/html
					echo -e "\e[1;36m\n*\n* COPY DOCS project ${sd} into \"$docsdir\" ...\n*\e[0m"
	
					if [ ! -d $docsdir ]; then
						mkdir -p $docsdir
					fi

					if [ -x $docsdir ]; then
						rm -rf $docshtmldir
						cp -a doc/html $docsdir/
						if [ $? -ne 0 ]; then
							exit 1
						fi
					else # if docs dir does not exist
						echo -e "\e[1;36m\n*\n* COPY DOCS \e[1;31mERROR\e[1;36m: directory \"$docsdir\" does not exist!\n*\e[0m"
						exit 1
					fi # -x $docsdir

				fi # qmake and make successful			
			fi # docs -eq 1		
		fi ## .pro file exists
		
		cd ..
	done # for
	cd ..
done

# copy scripts/cusetenv.sh and scripts/config.sh into $build_dir/bin
if [ ! -d $build_dir/bin ]; then
    mkdir -p $build_dir/bin
fi
cp -a scripts/cusetenv.sh scripts/config.sh $build_dir/bin

# restore prefix in meson.build and .pro files

# fix meson.build prefix
echo -e -n "\n... \e[0;32mrestoring\e[0m meson projects -Dprefix=$install_prefix -Dlibdir=$lib_dir -Dbuildtype=$build_type\e[0m in all meson projects..."
find . -type d -name "builddir" -exec meson configure -Dprefix=$install_prefix -Dlibdir=$lib_dir -Dbuildtype=$build_type  {} \;
echo -e "\t[\e[1;32mdone\e[0m]\n"

# fix all qmake INSTALL_ROOT that in the build phase used to point to build_dir
echo -e -n "\n... \e[0;32mrestoring\e[0m qmake INSTALL_ROOT=$install_prefix\e[0m in all Qt projects..."
find . -name "*.pro" -execdir qmake INSTALL_ROOT=$install_prefix  \; &>/dev/null
echo -e "\t[\e[1;32mdone\e[0m]\n\n"

# Replace any occurrence of $build_dir in any file (e.g. pkgconfig files)
echo -e -n "\n\n\e[1;32m* \e[0mreplacing temporary install dir references in \e[1;2mpkgconfig files\e[0m with \"\e[1;2m$install_prefix\e[0m\"..."
find $build_dir -type f -name "*.pc" -exec  sed -i 's#'"$build_dir"'#'"$install_prefix"'#g' {} +
echo -e "\t[\e[1;32mdone\e[0m]\n"

echo -e "\e[1;35mchecking\e[0m for temporary install dir references in files under \e[1;2m$build_dir\e[0m:"
echo -e "[ \e[1;37;3mgrep -rI  "$build_dir"  $build_dir/* \e[0m]"
echo -e "\e[0;33m+----------------------------------------------------------------------------------------+\e[0m"
echo -e "\e[1;31m"

# grep -rI r: recursive I ignore binary files
#
grep -rI "$build_dir" $build_dir/*
#
echo -e "\e[0;33m+----------------------------------------------------------------------------------------+\e[0m"
echo -e "\e[0m [\e[1;35mcheck \e[0;32mdone\e[0m] (^^ make sure there are no red lines within the dashed line block above ^^)"


qt_prf_file=$build_dir/include/cumbia-qt.prf

# copy cumbia-qt.prf.in into include dir as cumbia-qt.prf
cp -a cumbia-qt.prf.in $qt_prf_file

# Set the correct INSTALL_ROOT in $build_dir/cumbia-qt.prf:
sed -i 's|'INSTALL_ROOT[[:space:]]=[[:space:]].*'|INSTALL_ROOT = '$install_prefix'|g' $qt_prf_file



if [ $make_install -eq 0 ] && [ $build -eq 1 ]; then
    echo  -e "Type \e[1;32m$0 install\e[0m to install the library under \e[1;32m$install_prefix\e[0m\n"
fi


# INSTALL  SECTION
#
#
#
if [ $make_install -eq 1 ] && [ -r $build_dir ] &&  [ "$(ls -A $build_dir)" ]; then

        # A) Create install dir
        if [ ! -r $install_prefix ]; then
            echo -e "\n The installation directory \"$install_prefix\" does not exist. "
            echo -n  -e " Do you want to create it (the operation may require administrative privileges - sudo) [y|n]?  [y] "
            read  -s -n 1 createdir
            if [ "$createdir" != "y" ]  && [ "$reply" != "createdir" ] && [ "$createdir" != "" ]; then
                        exit 1
            fi

            # 1. try as normal user
            mkdir -p $install_prefix
            if [ "$?" -ne 0 ]; then
                if  [[ ! -z  $sudocmd  ]]; then
                        echo -e " The \e[1;32msudo\e[0m password is required to create the directory \"$install_prefix\""
                fi
                $sudocmd mkdir -p $install_prefix
                if [ "$?" -ne 0 ]; then
                        echo -e " \e[1;31merror\e[0m: failed to create installation directory \"$install_prefix\""
                        exit 1
                fi
            fi # [ "$?" -ne 0 ]

         else
            echo -e "\ndestination directory \"\e[1;32;4m$install_prefix\e[0m\" already exists\t[\e[1;32mOK\e[0m]"
         fi

        # B) Copy files

        echo -e -n "\n... \e[0;32mcopying\e[0m files into destination folder \"\e[1;32;4m$install_prefix\e[0m\"..."

        # 1. try install as normal user
        install_ok=0
        if [ -w $install_prefix ] ; then
            cp -a $build_dir/* $install_prefix/
        else # 2. no permissions? --> install with sudo
            if  [[ ! -z  $sudocmd  ]]; then
                    echo -e " The \e[1;32msudo\e[0m password is required to install the library under \"$install_prefix\""
            fi
            $sudocmd cp -a $build_dir/* $install_prefix/
            if [ "$?" -ne 0 ]; then
                    echo -e "\t[\e[1;31merror\e[0m]\nfailed to install into directory \"$install_prefix\""
                    exit 1
            else
                install_ok=1
            fi # [ "$?" -ne 0 ]
        fi # [ -w $install_prefix ]

        if [ $install_ok -eq 1 ]; then
            echo -e "\t[\e[1;32msuccess\e[0m]\n"
            echo -e "The following modules have been installed:"

            for f in `ls tmp-build-dir`; do
                echo -e  "\e[1;32;4m$f\e[0m - built on `cat tmp-build-dir/$f`"
            done
        fi # install_ok

        libprefix=$install_prefix/lib

        if [[ -z `ldconfig -v 2>/dev/null | grep ':$' |sed -e 's/://' | grep "^$libprefix$"` ]]; then
		echo -e "\e[0;33m\n*\n* INSTALL \e[1;33mWARNING \e[0mit is possible that \e[0;4m$libprefix\e[0m is not among the ld.so.conf paths: please check.\e[0m"

                # remove trailing "/" from $install_prefix, if present
                prefix_cl=${install_prefix%/}
		if [[ "$prefix_cl" == "/usr/local" ]] || [[ "$prefix_cl" == "/usr" ]] ; then
                        echo -e "\e[0;33m*\n* INSTALL \e[1;33mWARNING \e[0m check if under $install_prefix (your prefix) there are symbolic links between *lib* and *lib64*, e.g."
                        echo -e "\e[0;33m*\e[0m                  $install_prefix/lib --> $install_prefix/lib64"
                fi
                echo -e ""
                echo -e "\e[0;33m*\n* INSTALL \e[1;32mINFO \e[0mYou can execute \e[1;32msource $DIR/cusetenv.sh\e[0m to use the new libraries in the current shell"
                echo -e ""
		echo -e "\e[0;33m*\n* INSTALL \e[1;33mWARNING \e[0mDo you want to add $libprefix to ld.so.conf paths by adding a file \e[0m"
		echo -e "\e[0;33m                  \e[0;4mcumbia.conf\e[0m under "/etc/ld.so.conf.d/"  [y|n] ? [n] \e[0m"
		read  -s -n 1 addconf
		if [ "$addconf" == "y" ]; then
				if [ ! -z $sudocmd ]; then
					echo -e  "\e[1;32msudo\e[0m authentication required:"
				fi

				echo -e "\e[0;32m*\n* INSTALL adding\e[1;32m"
		 		echo $libprefix | $sudocmd tee /etc/ld.so.conf.d/cumbia.conf
				echo -e "\e[0;32m*\n* to \e[1;32mldconfig\e[0;32m path"
				echo -e "\e[0;32m*\n* INSTALL executing \e[1;32mldconfig\e[0m\n"
				$sudocmd ldconfig 
				echo -e "\e[0;32m*\n* "
		else
			echo -e "\e[0;33m*\n* INSTALL \e[1;33mWARNING \e[0mcheck if \"$libprefix\" is in ld.so.conf path. If not, add it manually or type: "
			echo -e "  export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:$libprefix"
			echo -e "\e[0;33m*\n"
		fi	
        else
		echo -e "\e[0;32m\n*\n* INSTALL \e[1;32m$libprefix\e[0m has been found in ldconfig paths."
	fi

        binpath=$install_prefix/bin
        cumbia_bin_sh_path=/etc/profile.d/cumbia-bin-path.sh
        if [[ -z `echo $PATH |grep $binpath` ]]; then
               echo -e "\e[0;33m\n*\n* INSTALL \e[1;33mWARNING \e[0mit looks like \"$binpath\" is not in \e[0;33mPATH\e[0m:"
               echo -e "\e[0;33m*\e[0m                  \e[0;34m[\e[0m$PATH\e[0;34m]\e[0m\n"
               echo -e "\e[0;33m*\n* INSTALL \e[1;33mWARNING \e[0mDo you want to add \"\e[0;32m$binpath\e[0m\" to the system"
               echo -e "\e[0;33m*\e[0;33m*\e[0m                  PATH through a new file named \"cumbia-bin-path.sh\" under "
               echo -e "\e[0;33m*\e[0;33m*\e[0m                  \e[0;34m/etc/profile.d\e[0m [y|n] ?"

               read  -s -n 1 addbinpath
               if [ "$addbinpath" == "y" ]; then
                      if [ ! -z $sudocmd ]; then
                          echo -e  "\e[1;32msudo\e[0m authentication required:"
                       fi

                    if [ ! -d /etc/profile.d ]; then
                        $sudocmd mkdir -p /etc/profile.d
                    fi
                    # remove if file exists
                    if [ -f $cumbia_bin_sh_path ]; then
                        $sudocmd rm -f $cumbia_bin_sh_path
                    fi
                    # (re)create file
                    echo 'export PATH=$PATH:'"$binpath" | $sudocmd tee $cumbia_bin_sh_path
                    $sudocmd chmod +x $cumbia_bin_sh_path
                    echo -e "\e[1;32m*\e[0;32m\n* INSTALL \e[1;32myou may need to execute\n*\n  \e[1;36msource  /etc/profile\e[1;32m \n*"
                    echo -e "* to enable shortcuts for cumbia apps. Then type \n*\n  \e[1;36mcumbia\e[1;32m\n*\n* to see the available options\n*\e[0m"
               else
                    echo -e "\e[0;33m*\n* INSTALL consider adding \"$binpath\" to the \"PATH\" variable in your profile"
               fi
        else
             echo -e "\e[0;32m\n*\n* INSTALL \e[1;32m$binpath\e[0m has been found in PATH."
        fi # binpath in $PATH

	echo -e "\e[0;32m*\n* INSTALL cumbia installation is now complete\e[0m"
	
	if [ $tango -eq 1 ]; then
                echo -e "\e[1;32m*\n*\e[0m After making sure a TangoTest device instance is running on a given tango-db-host "
                echo -e "\e[1;32m*\e[0m and that the environment includes cumbia lib, plugins and bin paths (see below), type"
		echo -e "  export TANGO_HOST=tango-db-host:PORT" 
		echo -e "\e[1;32m*\e[0m and then"
		echo -e "  cumbia client sys/tg_test/1/double_scalar sys/tg_test/1/long_scalar"
                echo -e "\e[1;32m*\e[0m or"
                echo -e "  cumbia read sys/tg_test/1/double_scalar sys/tg_test/1/long_scalar"
                echo -e "\e[1;32m*\e[0m install the qumbia-tango-findsrc-plugin from https://github.com/ELETTRA-SincrotroneTrieste/qumbia-tango-findsrc-plugin.git"
                echo -e "\e[1;32m*\e[0m for Tango source bash auto completion"
		echo -e "\e[1;32m*\n*\e[0m"
	fi

	echo -e "\e[1;32m*\n* \e[1;34;4mDOCUMENTATION\e[0m: https://elettra-sincrotronetrieste.github.io/cumbia-libs/"
        echo -e "\e[1;32m*\n* \e[1;32;4mINFO\e[0m: execute \e[1;32msource $binpath/cusetenv.sh\e[0m to use the new libraries in the current shell"
	echo -e "\e[1;32m*\n*\e[0m"
	
elif  [ ! -r $build_dir ] && [ $make_install -eq 1 ];  then
    echo -e "\e[1;31merror\e[0m: temporary install directory not found. Try executing once again"

fi

#
## restore original PKG_CONFIG_PATH
#
export PKG_CONFIG_PATH=$save_pkg_config_path
