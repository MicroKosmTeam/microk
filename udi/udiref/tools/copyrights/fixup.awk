
#
# Find the top-level udiproto directory in this tree.
#
function topdir(FNAME) {
 	FNAME=""
 	while(1) {
 		if (!system("test -d " FNAME"../udiref" )) {
 			return FNAME
 		}
 		FNAME=FNAME"../"
 	}
}

BEGIN {
	MY_BLOCK="\\$Copyright"
}



$0 ~ MY_BLOCK {
		FULL_ORIG_LINE=$0

		# Delete our starter comment

		sub(MY_BLOCK, "")
		# Extract the word following the comment block
		FTYPE=substr($0, match($0,/[_a-z]+/), RLENGTH)
		ORIG_LINE=$0

		COM_PREFIX=substr($0, 1, match($0, FTYPE) -2 )

		# Strip the backslash out of our comment prefix, output
		# that as the first line of our comment block.
		FLINE =  COM_PREFIX MY_BLOCK " "  FTYPE ":"
		gsub(/\\/, "", FLINE)
		print FLINE

		TOPDIR=topdir(FILENAME)
		COPYRIGHT=TOPDIR"tools/copyrights/msgs/"

		# Build the full pathname to the copyright file to use.
		COPYRIGHT_FILE = COPYRIGHT "/" FTYPE  

		# Prove copyright file exists.
		if (system("test -r " COPYRIGHT_FILE)) {
			print "Cannot open " COPYRIGHT_FILE
			exit 1
		}
		

		# Display copyright file, prepending comment prefix
		# to each line as we go.
		while (getline < COPYRIGHT_FILE) {
			print COM_PREFIX $0
		}

		# So next file gets to read from the start.
		close (COPYRIGHT_FILE)

		# If our comment line doesn't contain the closing character,
		# eat the source file until we see one.

		if ( ORIG_LINE !~ /\$/ ) {
			while (getline) {
				if ( $0 ~ /\$/ ) {
					break;
				}
			}
		}
		# Output our trailer so we can be run again non-destructively
		# on the same source file.
		print COM_PREFIX "$"
		next
}
{
	print 
}
