#
# /usr/local/include is for libelf.
#
#FLAGS=-pthread -Wall -I/usr/local/include
FLAGS=-traditional-cpp -Wall -I/usr/local/include -DUDI_ABI_NON_ELF_UDIPROPS_SECTION

DBG=-gstabs+ #-O2

#
# /usr/local/lib is for libelf.
#
LIBS=-L/usr/local/lib -lelf -F/System/Library/PrivateFrameworks -framework electric-fence
#LIBS=-L/usr/local/lib -lelf -F/System/Library/PrivateFrameworks -framework electric-fence -l/System/Library/PrivateFrameworks/electric-fence.framework/Versions/Current/electric-fence
#LIBS=

# Use the UW one until we get a custom one for Darwin
TORTURE_RUN='../darwin-user/torture_run'
