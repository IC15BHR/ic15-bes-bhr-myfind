#!/bin/bash --norc
#
# @author Bernd Petrovitsch <bernd.petrovitsch@technikum-wien.at>
# @date $Date: 2015-03-23 14:12:21 +0100 (Mon, 23 Mär 2015) $
#
# @version $Revision: 1451 $
#
# @todo
#
# URL: $HeadURL: https://svn.petrovitsch.priv.at/ICSS-BES/trunk/2015/src/myfind/test-find.sh $
#
# Last Modified: $Author: bernd $
#

set -u          # terminate on uninitialized variables
#set -e          # terminate if command fails
#set -vx         # for debugging

# the known correct program
KNOWN_CORRECT_FIND=/usr/local/bin/bic-myfind
# guess what ...
QUIET=0
VERBOSE=0
FORCE=0
RV=0

readonly SIMPLEDIR="/var/tmp/test-find/simple"
readonly FULLDIR="/var/tmp/test-find/full"

readonly        CORRECT_STDOUT=`mktemp /tmp/known-correct-find-stdout.XXXXXXXXXX`
readonly CORRECT_STDOUT_SORTED=`mktemp /tmp/known-correct-sorted-find-stdout.XXXXXXXXXX`
readonly        CORRECT_STDERR=`mktemp /tmp/known-correct-find-stderr.XXXXXXXXXX`
readonly         TESTED_STDOUT=`mktemp /tmp/to-be-tested-find-stdout.XXXXXXXXXX`
readonly  TESTED_STDOUT_SORTED=`mktemp /tmp/to-be-tested-sorted-find-stdout.XXXXXXXXXX`
readonly         TESTED_STDERR=`mktemp /tmp/to-be-tested-find-stderr.XXXXXXXXXX`
readonly                DIFFED=`mktemp /tmp/diffed.XXXXXXXXXX`

     EMPH_ON="\033[1;33m"
EMPH_SUCCESS="\033[1;32m"
 EMPH_FAILED="\033[1;31m"
    EMPH_OFF="\033[0m"

SUCCESS_COUNT=0
FAILURE_COUNT=0

TOUSEDIR=${SIMPLEDIR}

#
# ---------------------------------------------------------------------------------------- functions ---
#

function show_usage() {
    echo "USAGE: $0 [-h]  [-q] [-v] [-f] [-c] [--color (auto|always|never)] [-t <path_to_the_to_be_tested_find>] [-r <path_to_the_known_correct_reference_find>]" >& 2
    echo "           -h: show this help" >& 2
    echo "           -q: do not show successful test results" >& 2
    echo "           -v: show a lot - probably only useful to debug the test script itself" >& 2
    echo "           -c: continue after failed tests" >& 2
    echo "           -f: use ${FULLDIR} instead of ${SIMPLEDIR} as test directory" >& 2
    echo "           --color: auto-detect, force or cease coloring" >& 2
}

function clean_up {
    :
#    rm -f "${CORRECT_STDOUT}" "${CORRECT_STDOUT_SORTED}" "${CORRECT_STDERR}" "${TESTED_STDOUT}" "${TESTED_STDERR}" "${TESTED_STDOUT_SORTED}" "${DIFFED}"
}

function finish {
    clean_up
    echo -e "${EMPH_SUCCESS}Successful${EMPH_OFF} Tests: ${SUCCESS_COUNT}"
    echo -e  "${EMPH_FAILED}Failed${EMPH_OFF}     Tests: ${FAILURE_COUNT}"
    trap - EXIT # reset it so that it does nothing
    exit "$RV"
}

function failed() {
    (( FAILURE_COUNT++ ))
    echo -e "$0: ${EMPH_FAILED}Test failed:${EMPH_OFF}" "$@"
    RV=1
    if [ "$FORCE" -eq 0 ]
    then
        exit 1
    fi
}

function success() {
    (( SUCCESS_COUNT++ ))
#    echo "SUCCESS_COUNT=${SUCCESS_COUNT}"
    if [ "$QUIET" -eq 0 ]
    then
        echo -e "$0: ${EMPH_SUCCESS}Test successful:${EMPH_OFF}" "$@"
    fi
}

function verbose() {
    if [ "$VERBOSE" -ne 0 ]
    then
        echo "running $@"
    fi
}

#
# ------------------------------------------------------------------------------------- process args ---
#
readonly SHORT_OPTS="hvqt:r:cf"
readonly LONG_OPTS="help,color:"
# print nothing and just check for errors terminating the script 

readonly CMDLINE=`getopt -o "$SHORT_OPTS" --longoptions "$LONG_OPTS" -- "$@"`
if [ $? != 0 ]; then
    show_usage
    exit 1
fi

eval set -- "$CMDLINE"

COLOR=auto
while [ "$#" -gt 0 ]; do
    case "$1" in
    -h|--help)
        show_usage
        exit 0
        ;;
    -c)
        FORCE=1
        shift
        ;;
    -f)
	TOUSEDIR="${FULLDIR}"
        shift
        ;;
    -q)
        QUIET=1
        shift
        ;;
    -v)
        VERBOSE=1
        shift
        ;;
    -t)
        TO_BE_TESTED_FIND="$2"
        echo "$0: Using to-be-tested binary \"$TO_BE_TESTED_FIND\""
        shift 2
        ;;
    -r)
        KNOWN_CORRECT_FIND="$2"
        echo "$0: Using known-correct reference binary \"$KNOWN_CORRECT_FIND\""
        shift 2
        ;;
    --color)
        COLOR="$2"
        shift 2
        ;;
    --)
        shift
        break
        ;;
    \?)
        echo "$0: Invalid option: -$1" >&2
        show_usage
        exit 1
        ;;
    :)
        echo "$0: Option -$1 requires an argument." >&2
        show_usage
        exit 1
        ;;
    *)
        echo "$0: Option -$2 is unknown." >&2
        show_usage
        exit 1
        ;;
    esac
done

if [ "$OPTIND" -le "$#" ]
then
    show_usage
    exit 1
fi

if [ -e "$TO_BE_TESTED_FIND" ]
then
    echo "$0: Using to-be-tested binary \"$TO_BE_TESTED_FIND\""
else
    echo "$0: Cannot find to-be-tested binary \"$TO_BE_TESTED_FIND\"" >&2
    show_usage
    exit 1
fi

if [ -e "$KNOWN_CORRECT_FIND" ]
then
    echo "$0: Using known-correct reference binary \"$KNOWN_CORRECT_FIND\""
else
    echo "$0: Cannot find known-correct reference binary \"$KNOWN_CORRECT_FIND\"" >&2
    show_usage
    exit 1
fi
case "$COLOR" in
    auto)
        if ! [ -t 1 ]; then # no colors if stdout is not on a tty
            EMPH_ON=""
            EMPH_SUCCESS=""
            EMPH_FAILED=""
            EMPH_OFF=""
        fi
        ;;
    never)
        EMPH_ON=""
        EMPH_SUCCESS=""
        EMPH_FAILED=""
        EMPH_OFF=""
        ;;
    always)
        ;; # do nothing
    *)
	echo "Unknown option to --color: \"$COLOR\"" >&2
        show_usage
        exit 1
        ;;
esac
readonly TO_BE_TESTED_FIND KNOWN_CORRECT_FIND QUIET VERBOSE
readonly EMPH_ON EMPH_SUCCESS EMPH_FAILED EMPH_OFF

#
# --------------------------------------------------------------------------------------- self-check ---

for dir in "$SIMPLEDIR" "$FULLDIR"
do
      if [ ! -d "$dir" ]
      then
          echo "$0: \"$dir\" does not exist. Did you built the test environment?"
          exit 1
      fi
done

#
# ----------------------------------------------------------------------------- install trap handler ---

trap "{ RV=1; echo \"$0: Terminated by signal - cleaning up ...\"; finish; }" SIGINT SIGTERM
trap '{ finish; }' EXIT

#
# --------------------------------------------------------------------- Make sure our files are gone ---
clean_up

#
# ------------------------------------------------- Make us extremely nice and ignore errors on that ---
# 
renice -n 19 "$$" 2>/dev/null || :
ionice -c "3" -n "7" -p "$$" 2>/dev/null || : 

#
# -------------------------------------------------------------------------------------------- Tests ---
#

# ------------------------------------------------------------------------------------------------------
echo -e "${EMPH_ON}----- Test 0.0: Test command line parameters - unknown options ----${EMPH_OFF}"
verbose "${TO_BE_TESTED_FIND}" "$TOUSEDIR" -die-option-gibt-es-nicht
if "${TO_BE_TESTED_FIND}" "$TOUSEDIR" -die-option-gibt-es-nicht >&/dev/null </dev/null
then
    failed "No exit code indicating failure when calling ${TO_BE_TESTED_FIND} with unknown options"
else
    success "Exit code indicating failure when calling ${TO_BE_TESTED_FIND} with unknown options"
fi

# ------------------------------------------------------------------------------------------------------
echo -e "${EMPH_ON}----- Test 0.1: Test command line parameters - superfluous options ----${EMPH_OFF}"

function test_param()
{
    verbose "${TO_BE_TESTED_FIND}" "$TOUSEDIR" "$@"
    if "${TO_BE_TESTED_FIND}" "$TOUSEDIR" "$@" >&/dev/null </dev/null
    then
        failed "No exit code indicating failure when calling ${TO_BE_TESTED_FIND} when providing additional parameters to \"${@%% *}\""
    else
        success "Exit code indicating failure when calling ${TO_BE_TESTED_FIND} when providing additional parameters to \"${@%% *}\""
    fi
}

test_param -name so nicht
test_param -path "$TOUSEDIR/so" nicht
test_param -user karl nicht
test_param -type d  nicht

for opt in -nouser -print -ls
do
    verbose "${TO_BE_TESTED_FIND}" "$TOUSEDIR" $opt so-nicht
    if "${TO_BE_TESTED_FIND}" "$TOUSEDIR" $opt so-nicht >&/dev/null </dev/null
    then
        failed "No exit code indicating failure when calling ${TO_BE_TESTED_FIND} when providing additional parameters to \"$opt\""
    else
        success "Exit code indicating failure when calling ${TO_BE_TESTED_FIND} when providing additional parameters to \"$opt\""
    fi
done

# ------------------------------------------------------------------------------------------------------
echo -e "${EMPH_ON}----- Test 0.2: Test command line parameters with a missing parameter ----${EMPH_OFF}"
for opt in -name -path -user -type 
do
    verbose "${TO_BE_TESTED_FIND}" "$TOUSEDIR" $opt
    if "${TO_BE_TESTED_FIND}" "$TOUSEDIR" $opt >&/dev/null </dev/null
    then
        failed "No exit code indicating failure when calling ${TO_BE_TESTED_FIND} when providing no parameters to \"$opt\""
    else
        success "Exit code indicating failure when calling ${TO_BE_TESTED_FIND} when providing no parameters to \"$opt\""
    fi
done

# ------------------------------------------------------------------------------------------------------
echo -e "${EMPH_ON}----- Test 0.3: Test command line parameters with a wrong parameter ----${EMPH_OFF}"
test_param -type 'x'

# ------------------------------------------------------------------------------------------------------
echo -e "${EMPH_ON}----- Test 1.0: Test with simple and single files ----${EMPH_OFF}"

function run_command()
{
    local -r stdoutfilename="$1"
    local -r stderrfilename="$2"
    shift 2
    local -r command="$1"
    # and the rest are the parameters

    verbose "$@"
    "$@" > "$stdoutfilename" 2> "$stderrfilename"
    local -r rc="$?"
    if [ "$rc" -eq "0" ]
    then
        if [ -s "$stderrfilename" ]
        then
                failed "Command \"$@\" succeeded but data on stderr in $stderrfilename."
        fi
    else
        if [ ! -s "$stderrfilename" ]
        then
                failed "Command \"$@\" failed but no error message on stderr."
        fi        
    fi
    if egrep -v "^[[:space:]]*[[]?(${command}|${command##*/})[[:space:]]*(\]|:)" "$stderrfilename"
    then
        failed "Error messages are not preceeded with the commands name"
    fi
    return "$rc"
}

function diff_files() {
    local -r f1="$1"
    local -r f2="$2"
    shift 2
    if diff --ignore-space-change -U0 -N "$f1" "$f2" > "$DIFFED"
    then
        success "The output for \"$@\" is similar."
    else
        cat "$DIFFED"
        failed "The output for \"$@\" is wrong. $f1 $f2"
    fi
    rm "$DIFFED"
}

# "wc -l" needs a fork(2)+execve(2)
function count_lines()
{
    local i=0
    while read line
    do
      (( i++ ))
    done
    echo "$i"
}

function test_single_file()
{
    local findrc myfindrc finderrlines myfinderrlines
    # silently ignore files in directories which we cannot see
    while read file
    do
        run_command "$CORRECT_STDOUT" "$CORRECT_STDERR" "$KNOWN_CORRECT_FIND" "$file" "$@"
        findrc="$?"

        run_command "$TESTED_STDOUT" "$TESTED_STDERR" "$TO_BE_TESTED_FIND" "$file" "$@"
        myfindrc="$?"

        # check the line count of the error file
        finderrlines=$(count_lines < "$CORRECT_STDERR")
        myfinderrlines=$(count_lines < "$TESTED_STDERR")
        if [ "$finderrlines" -ne "$myfinderrlines" ]
        then
            failed "The error line count for \"$file $@\" are not equivalent - $finderrlines vs $myfinderrlines."
        fi
        # check the return values
        if [ "$findrc" -eq "0" -a "$myfindrc" -eq 0 ] || [ "$findrc" -ne "0" -a "$myfindrc" -ne "0" ]
        then
            # the have the same return value. now check the output
            diff_files "$CORRECT_STDOUT" "$TESTED_STDOUT" "$file" "$@"
        else
            failed "The return values for \"$file $@\" are not equivalent - $findrc vs $myfindrc."
        fi
    done < <(find "$TOUSEDIR" -type f 2> /dev/null) 
}

# and for which options should we test?

# we build a list as we need it later on anyways
declare -ar single_opt_00=('')
declare -ar single_opt_01=(-user "karl")
declare -ar single_opt_02=(-user "hugo")
declare -ar single_opt_03=(-user "150")
declare -ar single_opt_04=(-user "160")

declare -ar single_opt_10=(-nouser)

declare -ar single_opt_20=(-name "so")
declare -ar single_opt_21=(-name "*file")

declare -ar single_opt_30=(-path "$TOUSEDIR/so")
declare -ar single_opt_31=(-path "*hidden")

declare -ar single_opt_40=(-type "b")
declare -ar single_opt_41=(-type "c")
declare -ar single_opt_42=(-type "d")
declare -ar single_opt_43=(-type "p")
declare -ar single_opt_44=(-type "f")
declare -ar single_opt_45=(-type "l")
declare -ar single_opt_46=(-type "s")

declare -ar single_opt_50=(-print)
declare -ar single_act_50=(-print)
declare -ar single_opt_51=(-ls)
declare -ar single_act_51=(-ls)

if true; then
# and do something: first, just one of them
for var in ${!single_opt_*}
do
    # pass all of the array elements
    eval test_single_file "\${$var[*]}"
done

# ------------------------------------------------------------------------------------------------------
echo -e "${EMPH_ON}----- Test 2.0: Test with simple and single files with two options ----${EMPH_OFF}"

# and do something: then 2 of them
for var1 in ${!single_opt_*}
do
    for var2 in ${!single_opt_*}
    do
        # pass all of the array elements
        eval test_single_file "\${$var1[*]}" "\${$var2[*]}"
    done
done

# ------------------------------------------------------------------------------------------------------
echo -e "${EMPH_ON}----- Test 3.0: Test with simple and single files with two output-only options and three all of them ----${EMPH_OFF}"

# now compose 3 output actions with 2 of all
for var1 in ${!single_act_*}
do
  for var2 in ${!single_opt_*}
  do
    for var3 in ${!single_act_*}
    do
      for var4 in ${!single_opt_*}
      do
        for var5 in ${!single_act_*}
        do
            # pass all of the array elements
            eval test_single_file "\${$var1[*]}" "\${$var2[*]}" "\${$var3[*]}" "\${$var4[*]}" "\${$var5[*]}"
        done
      done
    done
  done
done

exit 0
# the below is untested ....
fi

# ------------------------------------------------------------------------------------------------------
echo -e "${EMPH_ON}----- Test 4.0: Test the whole simple directory ----${EMPH_OFF}"

function test_directory()
{
    run_command "$CORRECT_STDOUT" "$CORRECT_STDERR" "$KNOWN_CORRECT_FIND" "$@"
    local -r findrc="$?"

    run_command "$TESTED_STDOUT" "$TESTED_STDERR" "$TO_BE_TESTED_FIND" "$@"
    local -r myfindrc="$?"

    # check the return values
    if [ "$findrc" -eq "0" -a "$myfindrc" -eq "0" ] || [ "$findrc" -ne "0" -a "$myfindrc" -ne "0" ]
    then
        # the have the same return value. now check the output
        # sort the output files
        sort < "$CORRECT_STDOUT" > "$CORRECT_STDOUT_SORTED"
        sort < "$TESTED_STDOUT"  > "$TESTED_STDOUT_SORTED"

        diff_files "$CORRECT_STDOUT_SORTED" "$TESTED_STDOUT_SORTED" "$@"
    else
        failed "The return values for \"$directory $@\" are not equivalent - $findrc vs $myfindrc."
    fi
}

# and now over the whole simple directory hierarchy
test_directory "$TOUSEDIR" # no parameters ...

# and do something: first, just one of them
for var in ${!single_opt_*}
do
    # pass all of the array elements
    eval test_directory "$TOUSEDIR" "\${$var[*]}"
done

# Local Variables:
# sh-basic-offset: 4
# End:

