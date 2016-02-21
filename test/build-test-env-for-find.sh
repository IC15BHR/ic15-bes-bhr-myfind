#!/bin/bash --norc
#
# Bernd Petrovitsch <bernd.petrovitsch@technikum-wien.at>
#
# $Id: build-test-env-for-find.sh 1435 2015-03-22 15:40:57Z bernd $
#
# Macht eine Directoryhierarchie um `find` zu testen
#

set -ue
#set -vx # for debugging

echo "Please customize the variables in the script!"; exit 0
#and comment then the previous line out!

if [ "$(id -u)" -ne 0 ]; then
	echo "You must be root run this script!"
        exit 1
fi

# customizing
readonly TOPDIR="/var/tmp/test-find" # where to install that
readonly NUM_USERNAME="160"
readonly NUM_UID="150"
readonly NUM_OTHERUSERNAME="karl"
readonly GECOS="test-env-for-find"
readonly NOT_USED_UID="999999"
# check it
readonly SAVE_IFS="$IFS"
IFS=":"
while read username pw uid gid gecos homedir loginshell
do
  if [ "$uid" = "$NOT_USED_UID" ]
  then
      echo "UID $NOT_USED_UID exists. Use another one." >&2
      exit 1
  fi
done < /etc/passwd
IFS="$SAVE_IFS"

# the following will generate
#/etc/passwd	karl:x:160:150:test-env-for-find:/home/karl:/bin/bash
#readonly PASSWD_KARL="${NUM_OTHERUSERNAME}:x:${NUM_USERNAME}:${NUM_UID}:${GECOS}:/home/${NUM_OTHERUSERNAME}:/bin/bash"
#/etc/passwd	160:x:150:150:test-env-for-find:/home/160:/bin/bash
#readonly PASSWD_160="${NUM_USERNAME}:x:${NUM_UID}:${NUM_UID}:${GECOS}:/home/${NUM_USERNAME}:/bin/bash"
#/etc/shadow	karl:!!:14659:0:99999:7:::
#readonly SHADOW_KARL="${NUM_OTHERUSERNAME}:\!\!:14659:0:99999:7:::"
#/etc/shadow	160:$1$sVxlJcR2$oFklp.YKZYc3x9LjDiKfN/:13640:0:99999:7:::
#readonly SHADOW_160="${NUM_USERNAME}:\$1\$sVxlJcR2\$oFklp.YKZYc3x9LjDiKfN/:13640:0:99999:7:::"
#/etc/group	160:x:150:
#readonly GROUP_160="${NUM_USERNAME}:x:${NUM_UID}:"
#/etc/gshadow	160:!::
#readonly GSHADOW_160="${NUM_USERNAME}:\!::"
create_or_update_users() {
    # uids
    # add them to /etc/passwd if necessary. Check that we do not overwrite anything.
    # we mark our own with special text in the gecos field
    if ! awk -F: '$1 == NUM_OTHERUSERNAME && $5 == GECOS {exit 1}' NUM_OTHERUSERNAME="${NUM_OTHERUSERNAME}" GECOS="${GECOS}" /etc/passwd ||
	! awk -F: '$1 == NUM_USERNAME      && $5 == GECOS {exit 1}' NUM_OTHERUSERNAME="${NUM_OTHERUSERNAME}" GECOS="${GECOS}" /etc/passwd; then
        # if we have partly our lines, delete them
	userdel "${NUM_OTHERUSERNAME}" 
	userdel "${NUM_USERNAME}"
    fi
    # if we do not clash (either because we just deleted them or they were never there), add them
    if awk -F: '$1 == NUM_OTHERUSERNAME || $1 == NUM_USERNAME {exit 1}' NUM_OTHERUSERNAME="${NUM_OTHERUSERNAME}" NUM_USERNAME="${NUM_USERNAME}" /etc/passwd; then
        # "insert" the appropriate lines
	groupadd -g "${NUM_UID}" "${NUM_UID}" || : # ignore errors
	useradd --uid "${NUM_UID}"      --gid "${NUM_UID}" --comment "${GECOS}" --no-user-group "${NUM_USERNAME}"
	useradd --uid "${NUM_USERNAME}" --gid "${NUM_UID}" --comment "${GECOS}" --no-user-group "${NUM_OTHERUSERNAME}"
    fi
}

make_funny_files() {
    # create a (text) file
    echo "Hello world" > "plain-file"
    echo "Hello world" > ".hidden-file"
    # create directories
    mkdir "empty" "not-empty" ".empty-hidden" ".not-empty-hidden"
    # and some contents
    echo "Hello world again" > "not-empty/another-plain-file"
    echo "Hello world again" > "not-empty/.another-hidden-file"
    echo "Hello world again" > ".not-empty-hidden/another-plain-file"
    echo "Hello world again" > ".not-empty-hidden/.another-hidden-file"

    # create a file (containing only zeroes) with holes in it (a.k.a. sparse file)
    dd of="not-empty/file-without-holes" if="/dev/zero" bs=1024 count=10
    cp --sparse=always "not-empty/file-without-holes" "not-empty/file-with-holes"
# create a file with a size that is not an integral multiple of 1024
    dd of="not-empty/file-with-size-not-divisible-by-1024" if="/dev/zero" bs=512 count=13

# create hard-link
    ln "plain-file" "linked-plain-file"
    ln "plain-file" "not-empty/linked-plain-file"
# create sym-links
    ln -s "this-should-not-exist" "dangling-sym-link"
    ln -s "plain-file" "working-sym-link"
# and a long chain of sym-links
    for i in {2..9}; do
        ln -s "sym-link-$((${i} - 1))" "sym-link-${i}"
    done
    ln -s "plain-file" "sym-link-1"
}


make_long_link() {
# test with a long sym-link
    local LONG=""
    for i in {1..113}; do
        LONG="${LONG}$(uuidgen)"
    done
    ln -s "${LONG}" "long-link"
}

make_deep_directory() {
# und ein tiefes langes directory
    (
        set -e
        local pathname="$(uuidgen)"
        while [ "${#pathname}" -lt "4095" ]; do
            mkdir "$pathname"
            pathname="$pathname/$(uuidgen)"
        done
    ) || : # ignore errors
}

make_very_deep_directory() {
# und ein tiefes langes directory
    (
        local uuid
        set -e
        for i in {1..2048}
        do
          uuid="$(uuidgen)"
          mkdir "${uuid}"
          cd "${uuid}" 2> /dev/null
        done
    ) || : # ignore errors
}

umask 000

rm -rf "${TOPDIR}"
mkdir -p "${TOPDIR}/full" "${TOPDIR}/simple"
# fixup the permissions
chmod -R go-w "${TOPDIR}"
chown "$(id -u):$(id -g)" "${TOPDIR}" "${TOPDIR}/full" "${TOPDIR}/simple"

###################
# create or update user/group entries in /etc/passwd and /etc/group
#
create_or_update_users

###################
# generate a few simple test cases
#
cd "${TOPDIR}/simple"

: ein File, das fuer den Parametercheck genutzt wird  > so-nicht
chown "$NOT_USED_UID" "so-nicht" # und ein nicht existenter user ....
: noch ein File, das fuer den Parametercheck genutzt wird > so
: usercheck > "karl"
chown "karl" "karl" # und ein existenter user ....

make_funny_files

make_long_link
make_deep_directory

declare -a filetypes=(b c d f l p s)
i=0
# and a few files for the various types and permissions
for perms in u={r,w,x,s,xs},go= u=,g={r,w,x,s,xs},o= ug=,o={r,w,x,t,xt}
do
  case $(($i % 6)) in
      0) mknod "block-device-$perms" b 47 11
         chmod "$perms" "block-device-$perms"
         ;;
      1) mknod "char-device-$perms" c 08 15
         chmod "$perms" "char-device-$perms"
         ;;
      2) mkdir "directory-$perms"
         chmod "$perms" "directory-$perms"
         ;;
      3) mkfifo "fifo-$perms"
         chmod "$perms" "fifo-$perms"
         ;;
      4) : > "plain-file-$perms"
         chmod "$perms" "plain-file-$perms"
         ln -sf "plain-file-$perms" "sym-link-$perms"
         chmod "$perms" "sym-link-$perms"
         ;;
      5) mksock "socket-$perms"
         chmod "$perms" "socket-$perms"
         ;;
  esac
  i=$(( $i + 1 ))
done


###################
# build a quite large set of files and the like with lots of combinations
#
cd "${TOPDIR}/full"

make_funny_files

make_long_link
make_very_deep_directory

# block, char device and a fifo
mknod "block-device" "b" 999 999
mknod "char-device"  "c" 998 998
mkfifo "named-pipe"

# play permission games
for perms in u={r,-}{w,-}{x,-}{s,},g={r,-}{w,-}{x,-}{s,},o={r,-}{w,-}{x,-}{t,}; do
    mknod "block-device-$perms" b 47 11
    mknod "char-device-$perms" c 08 15
    mkdir "directory-${perms}"
    mkfifo "fifo-$perms"
    : > "plain-file-${perms}"	# save fork(2)+exec(2) avoiding "touch"
    ln -sf "plain-file-$perms" "sym-link-$perms"
    mksock "socket-$perms"
    chmod "${perms//-}" "block-device-$perms" "char-device-$perms" "directory-${perms}" "fifo-$perms" "plain-file-${perms}" "sym-link-$perms" "socket-$perms"
done

# now create some more files
: > "test-${NUM_USERNAME}"	# save fork(2)+exec(2) avoiding "touch"
: > "test-${NUM_OTHERUSERNAME}"	# save fork(2)+exec(2) avoiding "touch"
chown "${NUM_USERNAME}:${NUM_UID}" "test-${NUM_USERNAME}"
chown "${NUM_OTHERUSERNAME}:${NUM_UID}" "test-${NUM_OTHERUSERNAME}"

exit 0
