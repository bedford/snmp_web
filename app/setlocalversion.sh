#!/bin/sh
# Print additional version information for non-release trees.

usage() {
        echo "Usage: $0 [srctree]" >&2
        exit 1
}

cd "${1:-.}" || usage

# Check for git and a git repo.
if head=`git rev-parse --verify HEAD 2>/dev/null`; then
	# Do we have an untagged version?
	if git name-rev --tags HEAD | grep -E '^HEAD[[:space:]]+(.*~[0-9]*|undefined)$' > /dev/null; then
		#printf '%s' -
	        printf '%s' `echo "$head" | cut -c1-8`
	else
	    #printf '%s' - `git name-rev --tags HEAD | awk '{print $2}' | awk -F'/' '{print $2}'`
	    printf '%s' `git name-rev --tags HEAD | awk '{print $2}' | awk -F'/' '{print $2}'`
	fi
	#printf '%s' -g

	git diff > /dev/null

	# Are there uncommitted changes?
	if git diff-index HEAD | read dummy; then
		printf '%s' -
		printf '%s' `date +"%Y%m%d_%H%M"`
	fi
fi

# Check for svn and a svn repo.
if rev=`svn info 2>/dev/null` ; then
        res=`echo "${rev}" | grep '^Last Changed Rev' | awk '{print $NF}'`
	if [ -z "$res" ] ; then
            res=`echo "${rev}" | grep '^最后修改的版本' | awk '{print $NF}'`
	fi
        printf -- '-svn-%s' $res
fi

if mod=`svn status -q 2>/dev/null` ; then
        mod=`echo "${mod}" | awk '$1 ~ /^[ADCM]/ {printf "haha "}' | awk '{printf NF}'`
	if [ -n "$mod" ] ; then
		printf '%s' -dirty
		printf '%s' `date +"%Y%m%d_%H%M"`
        fi
fi
