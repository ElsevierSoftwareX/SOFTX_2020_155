#!/usr/bin/wish

;# This is code version; displayed in the About dialog box, Help menu
set daqconfig_version 1.0

;# Only support UNIX
switch $::tcl_platform(platform) {
    default { set answer [tk_messageBox -message "Windows, etc. unsupported!" -type ok ]; exit }
    unix {;}
}

;# Create menu
;# Tempporary stub
proc menu_clicked { no opt } {
    tk_messageBox -message \
	"You have clicked $opt.\nThis function is not implanted yet."
}

;# Declare that there is a menu
menu .mbar
. config -menu .mbar

;# The Main Buttons
.mbar add cascade -label "File" -underline 0  -menu [menu .mbar.file -tearoff 0]
.mbar add cascade -label "Help" -underline 0  -menu [menu .mbar.help -tearoff 0]

;# File Menu
set m .mbar.file
$m add command -label "Save" -underline 0 -command { menu_clicked 1 "Save" }
$m add separator
$m add command -label "Exit" -underline 1 -command exit

;# Help
set m .mbar.help
$m add command -label "About" -command { 
    tk_messageBox -default ok -icon info -message "DAQ configuration file editor\nversion $daqconfig_version" -title About -type ok;
}

#Making a text area
#text .txt -yscrollcommand ".srl set"
#scrollbar .srl -command {.txt yview}
#pack .txt -expand 1 -fill both -side left
#pack .srl -expand 1 -fill y


;# See what configuration directories exist
set dir "/cvs/cds/*/chans/daq/*.ini"
set files {};
set ro_files {};

;# Do not show read-only files
foreach file [glob -nocomplain $dir] {
    if {[file writable $file]} {
	lappend files $file
    } else {
	lappend ro_files $file;
    }
}


;# Nothing to do if there are no config files
if {[llength $files] == 0 } {
  tk_messageBox -message "No config files found!" -type ok -icon info
  exit
}



;# puts $files;
set oopt [lindex $files 0]

;# Create option menu to decide which file to edit
set omenu [eval "tk_optionMenu .omn oopt  $files"]
for {set i 0} {$i <= [$omenu index last]} {incr i} {
    $omenu entryconfigure $i -command repack_main_layout;
}
pack .omn
;# Create frame where to keep all dynamic widgets
frame .frm
pack .frm -fill x -fill y
#label .frm.test -text "Test"
#pack .frm.test

;# Called by the option menu when user changes her file selection
proc repack_main_layout {} {
    global file;
    ;#puts $::oopt;
    if {[winfo exists .lbl_cur_file]} {
	destroy .lbl_cur_file; # kill the label
	killChildren .frm; # kill all frame children widgets
    }
    ;# Create the widgets
    label .lbl_cur_file -text $::oopt
    #pack .lbl_cur_file
    set infile [open $::oopt r]
    set number 0
    global sections
    #global lws
    #destroy lws
    destroy sections
    while { [gets $infile line] >= 0 } {
	incr number
	if {[regexp {(#?)\s*\[\s*(\S+)\s*\]} $line foo comment section]} {
		;# New section starts
    		if {[info exists lws]} {
		  if {[string compare $lws "default"] != 0} {
		    pack .frm.$lws ;# Pack old section
		  }
		}
		set lws [string tolower $section]
		set lws [string map {"-" "_" ":" "_"} $lws]
		if { [string compare $section "default"] == 0 } {
		  set sections($lws,onoff) "on"
		  continue;
		}
		#puts "Section $lws"
		set sections($lws,onoff) on
		if {[string compare $comment "#"] == 0} {
		  set sections($lws,onoff) off
		}
		frame .frm.$lws 
		label .frm.$lws.label -text $section
		grid .frm.$lws.label -in .frm.$lws  -row 1 -column 2
		set wn [subst {.frm.$lws.onoff}]
		tk_optionMenu $wn sections($lws,onoff) on off
		#label .frm.$lws.label1 -text $comment
		grid .frm.$lws.onoff -in .frm.$lws  -row 1 -column 1
	} else {
		;# Section parameter
		if {[regexp {(#?)\s*(\S+)=(\S+)\s*} $line junk comment left right]} {
		   #puts "$comment$left = $right"
		   set sections($lws,$left)  $right;
		}
	}
    }
    pack .frm.$lws ;# Pack the last section
    close $infile
    #puts "$::oopt has $number lines";
     foreach key [lsort [array names sections]] {
	puts  "$key $sections($key)"
     }
}

;# Select the first available DAQ init file for editing
repack_main_layout

;# Destroy all parent window's children
proc killChildren {parent} {
   foreach w [winfo children $parent] {
      ;#if {[winfo class $w] eq "Checkbutton"} {
         destroy $w
      ;#}
   }
}
