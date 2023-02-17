#!/bin/sh
# File: env/common/udiget_common.sh
#
# Common functions for the udiget operation.  This file provides a set
# of shell functions and is designed to be included by the OS-specific
# udiget opeation to perform the standard operations for udiget.
#

#
# $Copyright udi_reference:
# 
# 
#    Copyright (c) 1995-2001; Compaq Computer Corporation; Hewlett-Packard
#    Company; Interphase Corporation; The Santa Cruz Operation, Inc;
#    Software Technologies Group, Inc; and Sun Microsystems, Inc
#    (collectively, the "Copyright Holders").  All rights reserved.
# 
#    Redistribution and use in source and binary forms, with or without
#    modification, are permitted provided that the conditions are met:
# 
#            Redistributions of source code must retain the above
#            copyright notice, this list of conditions and the following
#            disclaimer.
# 
#            Redistributions in binary form must reproduce the above
#            copyright notice, this list of conditions and the following
#            disclaimers in the documentation and/or other materials
#            provided with the distribution.
# 
#            Neither the name of Project UDI nor the names of its
#            contributors may be used to endorse or promote products
#            derived from this software without specific prior written
#            permission.
# 
#    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#    "AS IS," AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#    HOLDERS OR ANY CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
#    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
#    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
#    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
#    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
#    TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
#    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
#    DAMAGE.
# 
#    THIS SOFTWARE IS BASED ON SOURCE CODE PROVIDED AS A SAMPLE REFERENCE
#    IMPLEMENTATION FOR VERSION 1.01 OF THE UDI CORE SPECIFICATION AND/OR
#    RELATED UDI SPECIFICATIONS. USE OF THIS SOFTWARE DOES NOT IN AND OF
#    ITSELF CONSTITUTE CONFORMANCE WITH THIS OR ANY OTHER VERSION OF ANY
#    UDI SPECIFICATION.
# 
# 
# $
#

#
# copy_tree srcfiles outputdir
#
copy_tree() {
	for inp in $1 ; do
	    inpdir=`dirname $inp`
	    break
	done
	fd=`pwd`
	(cd $inpdir && find \
		`cd $fd && find $1 -prune -exec basename {} \;` -follow \
		-name bin -prune -o -print | cpio -padLmV $2) >/tmp/cte$$ 2>&1
	if [ -f /tmp/cte$$ ] ; then
	    grep -v 'newer or same age version exists' /tmp/cte$$ | \
	        egrep -v ' blocks|^Existing.*same age or newer$' 
	    rm -f /tmp/cte$$
	fi
}



#.......................................................................
#get_udi_source
#
# get_udi_source <input-location> <output-dir>
#
# Finds UDI source files corresponding to the input indication.
# The input-location must be one of:
#	* A UDI package filename
#       * [vendor:]shortname which is searched for in the udisetup
#         installed cache
#	* A directory containing the source and/or UDI package
# The source distribution is extracted and placed into output-dir
#
# stdout = errors
# return value=0 on success, non-zero on error

get_udi_source() {

    input=$1
    outdir=$2
    tmpdir=/tmp/gus_$$

    if [ -f $input ] ; then
	mkdir $tmpdir
	cp $input $tmpdir
	cd $tmpdir
	tar xf `basename $input`
	src=`find . -name src -type d -print`
	if [ "X$src" != "X" ] ; then
	    if [ -d $src ] ; then
		copy_tree "$src" $outdir
	    else
	        echo "Error: $input is corrupted: src is not a directory"
		cd /
		rm -rf $tmpdir
		return 2
	    fi;
	else
		echo "Error: $input is not a UDI pkg or does not contain source"
		cd /
		rm -rf $tmpdir
		return 2
	fi
    elif [ -d $input ] ; then
	if [ -f $input/udiprops.txt ] ; then
		copy_tree "$input/*" $outdir
	elif [ -f $input/src/udiprops.txt ] ; then
		copy_tree "$input/src/*" $outdir
	else
		echo "udiget: directory $input invalid as UDI source"
		return 2
	fi
    else
	udi_archdir="/etc/inst/UDI"
	src="$udi_archdir/`echo $input | sed -e 's|:|/|'`/src"
	if [ -d $src ] ; then
		copy_tree "$src" $outdir
	else
		echo "Unable to find installed source for UDI module $input"
		return 2
	fi
    fi
}


#.......................................................................
#build_udi_package
#
# build_udi_package <src-dir> [tools-loc,D=INSTALLED] [addtl-compile-opts] [udibuild-options] [udimkpkg-options]
#
# Builds a UDI package from the sources (+udiprops.txt) in the <src-dir>.
# Optionally, the location of the UDI tools to be used may be specified, and
# also optionally any additional compilation options may be specified.
#
# stdout = error code or name of UDI package file built


build_udi_package() {

    ( cd $1 || return 6

      if [ ! -f udiprops.txt ] ; then
          return 6
      fi

      etag="# DELETE: udiget_bup $3"

      if grep "$etag" udiprops.txt > /dev/null ; then
          # Properties already updated; don't touch it or we will force
          # a total rebuild.
          echo "$etag" > /dev/null;
      else

	if [ $# -ge 3 ] ; then  # additional compilation options

#	    if [ `grep module udiprops.txt | wc -l` != "1" ] ; then
#		    echo "No single module declaration in $input; can't handle this!"
#		    exit 3
#	    fi

	    # Insure we start off with a compile_options...
	    sed -e "/^release/a\\
compile_options
" udiprops.txt > udiprops_patched

	    # Now add fixes to any and all compile_options
	    sed -e "s/compile_options/compile_options $3/" \
			udiprops_patched > udiprops.txt
	fi

#	# Fixup module name
#	sed -e "s/module $1/module $outmod/" udiprops_fixed > udiprops_final

	# Enable POSIX TEST FRAME portions (kwq kwq)
	sed -e "/## POSIX TEST FRAME START/,/## POSIX TEST FRAME END/s/^#//" udiprops.txt > udiprops_patched
	mv udiprops_patched udiprops.txt
	echo "$etag" >> udiprops.txt

      fi

      if [ $# -ge 2 ] ; then  # additional compilation options
        if [ "X$2" != "XINSTALLED" ] ; then
          tooldir="$2/"
	else
	  tooldir=""
	fi
      else
        tooldir=""
      fi

      # Now build this module
      echo udibuild -vuD $4
      ${tooldir}udibuild -vuD $4 || {
		echo udibuild failed.
		exit 1
	}

      echo udimkpkg -O $5
      ${tooldir}udimkpkg -O $5 || {
		echo udimkpkg failed.
		exit 1
	}

      if [ -d $uditmp/udi-pkg.1 ] ; then
          echo `find $uditmp/udi-pkg.1 -name bin -print`/*/*
      else
	  echo ""
      fi
    )
}



#.......................................................................
#get_udipkg_props
#
# get_udipkg_props <UDI-pkg> <output-file>
#
# Obtains the static driver properties from a UDI package and places them in
# output-file.
#
# stdout = errors


get_udipkg_props () {

    pfn=`tar tf $1 | grep udiprops.txt`

    if [ "X$pfn" != "X" ] ; then

        tmpdir="/tmp/gpu_$$"
	curdir=`pwd`
	mkdir $tmpdir &&
	(cd $tmpdir;
	 pname=`echo "$curdir/$1" | sed -e 's!.*//!/!g'`
	 tar xf $pname $pfn
	) &&
	mv $tmpdir/$pfn $2
	rm -rf $tmpdir
	return 0

    fi

    echo "Cannot get binary properties yet..."
    return 5

}


#.......................................................................
#get_udipkg_msgs
#
# get_udipkg_msgs <UDI-pkg> <output-dir>
#
# Obtains the message files from a UDI package and places them in
# output-dir.  Any pre-existing message files in output-dir are
# overwritten; THERE IS NO PROTECTION FOR DIFFERENT PACKAGES WITH
# THE SAME MESSAGE FILE NAME.
#
# stdout = errors


get_udipkg_msgs () {

        tmpdir="/tmp/gpM_$$"
	curdir=`pwd`
	MF=""
	mkdir $tmpdir &&
	(cd $tmpdir;
	 pname=`echo "$curdir/$1" | sed -e 's!.*//!/!g'`
	 tar xf $pname
	) &&
	MF=`find $tmpdir -name msg -type d -exec find {} -print \;`
	if [ "X$MF" != "X" ] ; then
	    for mfile in $MF ; do
		if [ -r $mfile -a -f $mfile ] ; then
		    cp $mfile $2
		fi
	    done
        fi
	rm -rf $tmpdir
	return 0
}


#.......................................................................
#get_udipkg_mod
#
# get_udipkg_mod <UDI-pkg> <module> [ABI]
#
# Obtains the named module from the UDI package and places it into the
# current directory.
#
# stdout = errors


get_udipkg_mod () {

        tmpdir="/tmp/gpm_$$"
	curdir=`pwd`
	mkdir $tmpdir
	(cd $tmpdir;
	 pname=`echo "$curdir/$1" | sed -e 's!.*//!/!g'`
	 tar xf $pname
	)

	if [ $# -ge 3 ] ; then
	    binbase=`find $tmpdir -name bin -exec find {} -name $3 -print \;`
	else
	    binbase=`find $tmpdir -name bin -print`
	fi

	mv `find $binbase -name $2 -print` `pwd`/

	rm -rf $tmpdir
	return 0
}


