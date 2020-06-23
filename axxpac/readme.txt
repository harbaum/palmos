axxPac shell version 2.x / driver version 1.x
---------------------------------------------

The version 2.x of axxPac uses a driver library.
Please install at least axxPacShell.prc (the
file viewer) and axxPacDrv.prc (the axxPac driver 
library).

For information on the application programming 
interface read api.html in the api directory.

HISTORY
shell    2.0  - first release
         2.01 - card info view extended to cope
                with 64MBytes (4 zones)
         2.02 - UI tweaking
	 2.03 - redraw when card is changed
                during dialog/alert display
	 2.04 - new 'info' button	
	 2.05 - more UI tweaking
              - preferences save view, mode and path
	      - fixed some very old problem with 'rom preference'
	      - more flexible path handling
	 2.06 - changed plugin handling
	      - fixed problem with huge databases 
                (>8000 entries)
	 2.07 - included DA's to HACK group
	      - renamed explorer to shell due to
                legal issues
	 2.08 - extended plugin concept
	 2.09 - fixed problem with no axxPac installed
	 2.10 - cosmetic
	      - fixed problem with 0 bytes chunks
	 2.11 - fixed problem with slashes '/' in database names
	      - fixed problem with 'Null handles'
	 2.12 - new 'Move' button in file info dialog
	 2.13 - new 'Beam' feature in file info dialog
	 2.14 - fixed wrong error message
	 2.15 - sort files according to name/date+time/size
	 2.16 - bugfix in media info box
	      - new backup/restore functions
	      - 'move' from palm now deletes source
	      - delete of subdirs incl. containing files
	        (no subdirs of subdirs!)
	      - fixed display problem when deleting last file
                in subdirectory
	 2.17 - configurable excludes for backup
              - fixed problem with file info dialog
              - workaround for date bug with OS < 3.5 in 
	        database info dialog
	      - automatic backups
	 2.18 - include system preferences in backup
	 2.19 - card info now supports up to 8 zones (128MB)
	      - export of single memos as ASCII files to the smartmedia

driver   1.0  - first release
         1.01 - support for 64MBytes media
              - bug fix for 32MBytes
              - fixed problems when formating 
                defective card
         1.02 - bug fix 64 MByte media write
	      - formatting 64 MBytes works
              - media IO speedup
              - fixed some auto power down problem
         1.03 - format error handling 
	      - improved power management
              - new rename function
              - fixed bug that caused MSDOS incompatible
                root subdirectories to be created
         1.04 - fixed rename/create bug with subdirs
              - write protect bit handling
              - new SetWP function
	      - bug fix in 'file exists'
	 1.05 - bug fix in path handling
	      - error correction speedup 
	      - bug fix for short file deletion
	 1.06 - more little speedups
              - improved read handling
              - fast read for Ralph :-)
	 1.07 - new axxPacStat
              - new axxPacTell
	      - more speedup (now using burst reads)
	      - reduced wait states
                (read speed ~800kBytes/sec)
              - new file selector
	 1.08 - cosmetic changes
	 1.09 - improved file selector
	      - fixed problems with multiple apps 
                accessing the media concurrently
	 1.10 - fixed problems with 32 and 64 MBytes media
              - improved handling of huge dirs
	 1.11 - rewriting a file changes the modification date
	 1.12 - fixed lock-up, when card is removed during transfer 
	 1.13 - fixed bug when determining file creation time
	 1.14 - improved low mem handling in file selector
	 1.15 - fixed bug in change dir
	 1.16 - fixed format problems with defective sectors
	      - fixed bug when deleting long filenames
         1.17 - prevent moving of directory entries into its
                own subdirectories
	 1.18 - new axxPacSetAttrib
	      - new axxPacSetDate
	 1.19 - fixed name bug in driver offset table causing
	        trouble with multiple applications accessing the
		axxPac simultaneously
	 1.20 - fixed filename overflow in file selector
	 1.21 - added support for 128 MB media
	      - added support for fat16 filesystems
	      - re-design of memory handling
         1.22 - format function now creates Olympus panorama format





