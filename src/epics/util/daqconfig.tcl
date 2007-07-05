#!/usr/bin/wish

set daqconfig_version 1.0

#set filename [tk_chooseDirectory -initialdir /cvs/cds/caltech/chans/daq -mustexist true ]

#set answer [tk_messageBox -message "Really quit?" -type yesno -icon question]
# switch -- $answer {
    #yes exit
    #no {tk_messageBox -message "I know you like this application!" -type ok}
#}

# Only support UNIX
switch $::tcl_platform(platform) {
      default {
         set answer [tk_messageBox -message "Windows, etc. unsupported!" -type ok ]
	exit
      }
      unix {
        ;
      }
}

# Create menu
proc menu_clicked { no opt } {
	tk_messageBox -message \
		"You have clicked $opt.\nThis function is not implanted yet."
}

#Declare that there is a menu
menu .mbar
. config -menu .mbar

#The Main Buttons
.mbar add cascade -label "File" -underline 0 \
      -menu [menu .mbar.file -tearoff 0]
.mbar add cascade -label "Help" -underline 0 \
      -menu [menu .mbar.help -tearoff 0]

## File Menu ##
set m .mbar.file
$m add command -label "Save" -underline 0 -command { menu_clicked 1 "Save" }
$m add separator
$m add command -label "Exit" -underline 1 -command exit

## Help ##
set m .mbar.help
$m add command -label "About" -command { 
	tk_messageBox -default ok -icon info -message "DAQ configuration file editor\nversion $daqconfig_version" -title About -type ok;
}

#Making a text area
#text .txt -yscrollcommand ".srl set"
#scrollbar .srl -command {.txt yview}
#pack .txt -expand 1 -fill both -side left
#pack .srl -expand 1 -fill y


# See what configuration directories we have
set dir "/cvs/cds/*/chans/daq/*.ini";
set files {};
set ro_files {};
foreach file [glob -nocomplain $dir] {
      if {[file writable $file]} {
         lappend files $file
      } else {
	 lappend ro_files $file;
      }
}


if {[ llength $files] == 0 } {
#  pack .omn
  tk_messageBox -message "No config files found!" -type ok -icon info
  exit
}



#.txt insert end $files;
puts $files;
set oopt [lindex $files 0]
# Create option menu to decide which file to edit
set omenu [eval "tk_optionMenu .omn oopt  $files"]
#$omenu configure -command oopt_changed
for {set i 0} {$i <= [$omenu index last]} {incr i} {
   $omenu entryconfigure $i -command oopt_changed;
}

pack .omn

proc oopt_changed {} {
	puts $::oopt;
}
