#!/usr/bin/wish
#
# I am D. Richard Hipp, the author of this code.  I hereby
# disavow all claims to copyright on this program and release
# it into the public domain. 
#
#                     D. Richard Hipp
#                     January 31, 2001
#
# As an historical record, the original copyright notice is
# reproduced below:
#
# Copyright (C) 1997,1998 D. Richard Hipp
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
# 
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA  02111-1307, USA.
#
# Author contact information:
#   drh@acm.org
#   http://www.hwaci.com/drh/
#
# $Revision: 1.10 $
#
option add *highlightThickness 0

switch $tcl_platform(platform) {
  unix {
    set Tree(font) \
      -adobe-helvetica-medium-r-normal-*-11-80-100-100-p-56-iso8859-1
  }
  windows {
    set Tree(font) \
      -adobe-helvetica-medium-r-normal-*-14-100-100-100-p-76-iso8859-1
  }
}

#
# Create a new tree widget.  $args become the configuration arguments to
# the canvas widget from which the tree is constructed.
#
proc Tree:create {w args} {
  global Tree
  eval canvas $w -bg white $args
  bind $w <Destroy> "Tree:delitem $w /"
  Tree:dfltconfig $w /
  Tree:buildwhenidle $w
  set Tree($w:selection) {}
  set Tree($w:selidx) {}
}

# Initialize a element of the tree.
# Internal use only
#
proc Tree:dfltconfig {w v} {
  global Tree
  set Tree($w:$v:children) {}
  set Tree($w:$v:open) 0
  set Tree($w:$v:icon) {}
  set Tree($w:$v:tags) {}
}

#
# Pass configuration options to the tree widget
#
proc Tree:config {w args} {
  eval $w config $args
}

#
# Insert a new element $v into the tree $w.
#
proc Tree:newitem {w v args} {
  global Tree
  set dir [file dirname $v]
  set n [file tail $v]
  if {![info exists Tree($w:$dir:open)]} {
    error "parent item \"$dir\" is missing"
  }
  set i [lsearch -exact $Tree($w:$dir:children) $n]
  if {$i>=0} {
    error "item \"$v\" already exists"
  }
  lappend Tree($w:$dir:children) $n
  set Tree($w:$dir:children) [lsort $Tree($w:$dir:children)]
  Tree:dfltconfig $w $v
  foreach {op arg} $args {
    switch -exact -- $op {
      -image {set Tree($w:$v:icon) $arg}
      -tags {set Tree($w:$v:tags) $arg}
    }
  }
  Tree:buildwhenidle $w
}

#
# Delete element $v from the tree $w.  If $v is /, then the widget is
# deleted.
#
proc Tree:delitem {w v} {
  global Tree
  if {![info exists Tree($w:$v:open)]} return
  if {[string compare $v /]==0} {
    # delete the whole widget
    catch {destroy $w}
    foreach t [array names Tree $w:*] {
      unset Tree($t)
    }
  }
  foreach c $Tree($w:$v:children) {
    catch {Tree:delitem $w $v/$c}
  }
  unset Tree($w:$v:open)
  unset Tree($w:$v:children)
  unset Tree($w:$v:icon)
  set dir [file dirname $v]
  set n [file tail $v]
  set i [lsearch -exact $Tree($w:$dir:children) $n]
  if {$i>=0} {
    set Tree($w:$dir:children) [lreplace $Tree($w:$dir:children) $i $i]
  }
  Tree:buildwhenidle $w
}

#
# Change the selection to the indicated item
#
proc Tree:setselection {w v} {
  global Tree
  set Tree($w:selection) $v
  Tree:drawselection $w
}

# 
# Retrieve the current selection
#
proc Tree:getselection w {
  global Tree
  return $Tree($w:selection)
}

#
# Bitmaps used to show which parts of the tree can be opened.
#
set maskdata "#define solid_width 9\n#define solid_height 9"
append maskdata {
  static unsigned char solid_bits[] = {
   0xff, 0x01, 0xff, 0x01, 0xff, 0x01, 0xff, 0x01, 0xff, 0x01, 0xff, 0x01,
   0xff, 0x01, 0xff, 0x01, 0xff, 0x01
  };
}
set data "#define open_width 9\n#define open_height 9"
append data {
  static unsigned char open_bits[] = {
   0xff, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x7d, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0xff, 0x01
  };
}
image create bitmap Tree:openbm -data $data -maskdata $maskdata \
  -foreground black -background white
set data "#define closed_width 9\n#define closed_height 9"
append data {
  static unsigned char closed_bits[] = {
   0xff, 0x01, 0x01, 0x01, 0x11, 0x01, 0x11, 0x01, 0x7d, 0x01, 0x11, 0x01,
   0x11, 0x01, 0x01, 0x01, 0xff, 0x01
  };
}
image create bitmap Tree:closedbm -data $data -maskdata $maskdata \
  -foreground black -background white

# Internal use only.
# Draw the tree on the canvas
proc Tree:build w {
  global Tree
  $w delete all
  catch {unset Tree($w:buildpending)}
  set Tree($w:y) 30
  Tree:buildlayer $w / 10
  $w config -scrollregion [$w bbox all]
  Tree:drawselection $w
}

# Internal use only.
# Build a single layer of the tree on the canvas.  Indent by $in pixels
proc Tree:buildlayer {w v in} {
  global Tree
  if {$v=="/"} {
    set vx {}
  } else {
    set vx $v
  }
  set start [expr $Tree($w:y)-10]
  foreach c $Tree($w:$v:children) {
    set y $Tree($w:y)
    incr Tree($w:y) 17
    $w create line $in $y [expr $in+10] $y -fill gray50 
    set icon $Tree($w:$vx/$c:icon)
    set taglist x
    foreach tag $Tree($w:$vx/$c:tags) {
      lappend taglist $tag
    }
    set x [expr $in+12]
    if {[string length $icon]>0} {
      set k [$w create image $x $y -image $icon -anchor w -tags $taglist]
      incr x 20
      set Tree($w:tag:$k) $vx/$c
    }
    set j [$w create text $x $y -text $c -font $Tree(font) \
                                -anchor w -tags $taglist]
    set Tree($w:tag:$j) $vx/$c
    set Tree($w:$vx/$c:tag) $j
    if {[string length $Tree($w:$vx/$c:children)]} {
      if {$Tree($w:$vx/$c:open)} {
         set j [$w create image $in $y -image Tree:openbm]
         $w bind $j <1> "set Tree($w:$vx/$c:open) 0; Tree:build $w"
         Tree:buildlayer $w $vx/$c [expr $in+18]
      } else {
         set j [$w create image $in $y -image Tree:closedbm]
         $w bind $j <1> "set Tree($w:$vx/$c:open) 1; Tree:build $w"
      }
    }
  }
  set j [$w create line $in $start $in [expr $y+1] -fill gray50 ]
  $w lower $j
}

# Open a branch of a tree
#
proc Tree:open {w v} {
  global Tree
  if {[info exists Tree($w:$v:open)] && $Tree($w:$v:open)==0
      && [info exists Tree($w:$v:children)] 
      && [string length $Tree($w:$v:children)]>0} {
    set Tree($w:$v:open) 1
    Tree:build $w
  }
}

proc Tree:close {w v} {
  global Tree
  if {[info exists Tree($w:$v:open)] && $Tree($w:$v:open)==1} {
    set Tree($w:$v:open) 0
    Tree:build $w
  }
}

# Internal use only.
# Draw the selection highlight
proc Tree:drawselection w {
  global Tree
  if {[string length $Tree($w:selidx)]} {
    $w delete $Tree($w:selidx)
  }
  set v $Tree($w:selection)
  if {[string length $v]==0} return
  if {![info exists Tree($w:$v:tag)]} return
  set bbox [$w bbox $Tree($w:$v:tag)]
  if {[llength $bbox]==4} {
    set i [eval $w create rectangle $bbox -fill skyblue -outline {{}}]
    set Tree($w:selidx) $i
    $w lower $i
  } else {
    set Tree($w:selidx) {}
  }
}

# Internal use only
# Call Tree:build then next time we're idle
proc Tree:buildwhenidle w {
  global Tree
  if {![info exists Tree($w:buildpending)]} {
    set Tree($w:buildpending) 1
    after idle "Tree:build $w"
  }
}

#
# Return the full pathname of the label for widget $w that is located
# at real coordinates $x, $y
#
proc Tree:labelat {w x y} {
  set x [$w canvasx $x]
  set y [$w canvasy $y]
  global Tree
  foreach m [$w find overlapping $x $y $x $y] {
    if {[info exists Tree($w:tag:$m)]} {
      return $Tree($w:tag:$m)
    }
  }
  return ""
}

#################
#
# The remainder is code that demonstrates the use of the Tree
# widget.  
#
. config -bd 3 -relief flat
frame .f -bg white
pack .f -fill both -expand 1
image create photo idir -data {
    R0lGODdhEAAQAPIAAAAAAHh4eLi4uPj4APj4+P///wAAAAAAACwAAAAAEAAQAAADPVi63P4w
    LkKCtTTnUsXwQqBtAfh910UU4ugGAEucpgnLNY3Gop7folwNOBOeiEYQ0acDpp6pGAFArVqt
    hQQAO///
}
image create photo ifile -data {
    R0lGODdhEAAQAPIAAAAAAHh4eLi4uPj4+P///wAAAAAAAAAAACwAAAAAEAAQAAADPkixzPOD
    yADrWE8qC8WN0+BZAmBq1GMOqwigXFXCrGk/cxjjr27fLtout6n9eMIYMTXsFZsogXRKJf6u
    P0kCADv/
}

;# This is code version; displayed in the About dialog box, Help menu
set daqconfig_version {$Header: /var/svn/ldas-cvs/repository_cds/cds/advLigo/src/epics/util/daqconfig.tcl,v 1.10 2007/07/13 23:30:41 aivanov Exp $}

;# Only support UNIX
switch $::tcl_platform(platform) {
    default { set answer [tk_messageBox -message "Windows, etc. unsupported!" -type ok ]; exit }
    unix {;}
}

;# Site name
set site_name "caltech"
if { $argc > 0 } { set site_name [lindex $argv 0] }

;# See what configuration directories exist
set dir "/cvs/cds/${site_name}/chans/daq"
set files {};
set ro_files {};

;# Do not show read-only files
foreach file [glob -nocomplain -tails -directory $dir "*.ini"] {
    #if {[file writable $file]} {
        lappend files $file
    #} else {
        #lappend ro_files $file;
    #}
}


;# Nothing to do if there are no config files
if {[llength $files] == 0 } {
  tk_messageBox -message "No config files found!" -type ok -icon info
  exit
}

;# Create menu
;# Tempporary stub
proc menu_clicked { no opt } {
    tk_messageBox -message \
        "You have clicked $opt.\nThis function is not implanted yet."
}

;# Quit
proc quit_app {} {
    #set answer [tk_messageBox -message "Really quit?" -type yesno -icon question]
    #switch -- $answer {
       #no { return }
    #}
    exit
}

;# Save new configuration files
proc save_ini_files {} {
    global sections
    global section_names;
    if {[regexp {^/([^/]+)(/\S+)*$} $::current_tree_node foo fname] == 0} { return }
    #puts "Saving $fname"
    set answer [tk_messageBox -message "Really save $::dir/$fname?\n(backup archive file will be created)" -type yesno -icon question]
    switch -- $answer {
       no { return }
    }

    #foreach key [array names sections] {
      #if {[regexp "^$fname,(\[^,\]+),(\[^,\]+)" $key foo chname param]} {
        #puts  "$fname, $chname, $param $sections($key) key=$key"
      #}
    #}
    set infile [open "/tmp/$fname" w]

    ;# Write out default section first
    set default_written 0

    foreach i $section_names($fname) {
	if { $default_written == 1 } {
	   ;# Spacer after default section
    	   puts $infile ""
	   set defualt_written 2
	}
	set comment ""
  	if {[string compare  $sections($fname,$i,onoff) off] == 0} {
	  set comment "#"
 	}
	if {[string compare $i "default"] == 0} {
	  set default_written 1
	  ;# Default section is always enabled
	  puts $infile "\[$i\]"
        } else {
	  puts $infile "$comment\[${i}_$sections($fname,$i,datarate)\]"
        }
        foreach key [array names sections] {
           if {[regexp "^$fname,$i,(\[^,\]+)$" $key foo param]} {
	     set val $sections($key);
	     if {[string compare $param "onoff"] == 0} { continue }
	     if {[string compare $param "datatype"] == 0} { 
	  	     switch $val {
  		       "short" {
                   	set val 1
  		       }
 		       "float" {
                   	set val 4
		       }
		       default {
                   	set val 4
		       }
                     }
	     }
             puts $infile "$comment$param=$val"
           }
        }
    }
    close $infile
    ;# Create backup file
    #set infile [open "$::dir/$fname" r]
    set tm [clock format [clock seconds] -format %y%m%d_%H%M%S]
    set newfname "[file root $::dir/archive/$fname]_$tm.ini"
    #puts "mv $::dir/$fname $newfname"
    file rename -force -- $::dir/$fname $newfname
    file rename -force -- /tmp/$fname $::dir/$fname 
    tk_messageBox -message "$::dir/$fname saved. Backup is $newfname" -type ok
}

;# Declare that there is a menu
menu .mb
. config -menu .mb

;# The Main Buttons
.mb add cascade -label "File" -underline 0  -menu [menu .mb.file -tearoff 0]
.mb add cascade -label "Help" -underline 0  -menu [menu .mb.help -tearoff 0]

;# File Menu
set m .mb.file
$m add command -label "Save" -underline 0 -command { save_ini_files }
$m add separator
$m add command -label "Exit" -underline 1 -command { quit_app }

;# Help
set m .mb.help
$m add command -label "About" -command { 
    tk_messageBox -default ok -icon info -message "DAQ configuration file editor\n$daqconfig_version" -title About -type ok;
}

frame .f.tf 
;#-height 400 -width 300
label .f.tf.lbl1 -text "Files, Active Channels"
Tree:create .f.tf.w -yscrollcommand {.f.tf.sb set}
;# -width 300 -height 400 
scrollbar .f.tf.sb -orient vertical -command {.f.tf.w yview}
pack .f.tf.lbl1 -side top
pack .f.tf.w -side left -fill both -expand 1 -padx 5 -pady 5
pack .f.tf.sb -side left -fill y
frame .f.c
;#-height 400 -width 300
pack .f.tf -side left -fill both -expand 1
pack .f.c -side left -fill both -expand 1
#label .f.c.l -width 40 -text {} -bg [.f.c cget -bg]
#pack .f.c.l -expand 1

set current_tree_node ""

;# Destroy all parent window's children
proc killChildren {parent} {
   foreach w [winfo children $parent] {
      ;#if {[winfo class $w] eq "Checkbutton"} {
         destroy $w
      ;#}
   }
}

;# Parser ini file
proc parse_ini_file file {
    set infile [open "$::dir/$file" r]
    set number 0
    global sections
    global section_names
    set section_names($file) {}
    while { [gets $infile line] >= 0 } {
        incr number
        if {[regexp {(#?)\s*\[\s*(\S+)\s*\]} $line foo comment section]} {
                ;# New section starts
                #set lws [string tolower $section]
                #set lws [string map {"-" "_" ":" "_"} $lws]
		set lws $section
                #if { [string compare $section "default"] == 0 } {
                  #set sections($file,$lws,onoff) "on"
        	  #lappend section_names($file) $section
                  #continue;
                #}
                #puts "Section $lws"
                if {[regexp {^(\S+)_(\d+)$} $section junk channame chanrate]} {
			set lws $channame;
		}
                set sections($file,$lws,onoff) on
                if {[string compare $comment "#"] == 0} {
                  set sections($file,$lws,onoff) off
                }
        	lappend section_names($file) $lws
        } else {
                ;# Section parameter
                if {[regexp {(#?)\s*(\S+)=(\S+)\s*} $line junk comment left right]} {
                   #puts "$comment$left = $right"
                   set sections($file,$lws,$left)  $right;
                   set sections($file,$lws,$left,onoff) on
                   if {[string compare $comment "#"] == 0} {
                     set sections($file,$lws,$left,onoff) off
                   }
		   ;# Convert numeric datatype
		   if {[string compare $left "datatype"] == 0} { 
	  	     switch $right {
  		       1 {
                   	set sections($file,$lws,$left)  "short"
  		       }
 		       4 {
                   	set sections($file,$lws,$left)  "float"
		       }
		       default {
                   	set sections($file,$lws,$left)  "invalid"
		       }
                     }
                   }
		}
        }
    }
    close $infile
    #foreach key [lsort [array names sections]] {
      #puts  "$key $sections($key)"
    #}
#    foreach key [array names section_names] {
	#puts  "$key $section_names($key)"
    #}
    ;# Duplicate sections so we can check later whether need to save or not
}

;# Add selected channels to the list of active channels
proc add_channel {} {
	global sections
	set cur_sel [.f.c.l curselection]
	foreach i $cur_sel {
		set cn [.f.c.l get $i]
		# add this channel to the list of active channels
      		Tree:newitem .f.tf.w $::current_tree_node/$cn -image ifile
  		;# remove leading slash
  		set f [string range $::current_tree_node 1 end]
		# activate channel in the 'section'
		set sections($f,$cn,onoff) on
		#puts sections($f,$cn,onoff)
		#puts $sections($f,$cn,onoff)
 	}
	foreach i [lsort -decreasing $cur_sel] {
		# Delete channel name from the list of inactive channels
		.f.c.l delete $i
	}
}

;# Remove current channel from the list of active channels
proc remove_channel {} {
	global sections
	$puts $::current_tree_node
	;# Remove from the tree
	Tree:delitem .f.tf.w $::current_tree_node
	;# Split info file name and channel name
  	if {[regexp {/(\S+)/(\S+)} $::current_tree_node foo fname chname] == 0} { return }
	;# deactivate in 'sections'
	set sections($fname,$chname,onoff) "off"
	#puts sections($fname,$chname,onoff)
        #puts $sections($fname,$chname,onoff)

}


;# Calculate avilable DAQ channels rates based on maximum rate
proc get_rates maxrate {
	set rates {}
	while {$maxrate >= 16 } {
		lappend rates $maxrate
		set maxrate [expr $maxrate / 2]
	}
	return $rates;
}

;# Display channel parameters
proc display_params file {
  global sections
  global section_names
  set w .f.c

  ;# Split "file" into file name and channel name parts
  if {[regexp {/(\S+)/(\S+)} $file foo fname chname] == 0} { return }

  ;# Cleanup display frame first
  if {[winfo exists $w]} {
        killChildren $w
  }

  ;# Create scrollable list of channels
  if {[string compare $chname "default"] == 0} {
    label $w.lbl -text "Default Parameters"
    pack $w.lbl -side top
    label $w.lbl2 -text " "
    pack $w.lbl2 -side top
    ;# Print default section parameters
    foreach key [lsort [array names sections]] {
      if {[regexp "^$fname,$chname,(\[^,\]+)$" $key foo param]} {
	;# Do not display onoff parameter, meaningless
	if {[string compare $param "onoff"] == 0} { continue }
	set val $sections($key)
	;# See if this parameter is commented out
	if {[string compare $sections($fname,$chname,$param,onoff) "off"] == 0} {
	  set val "($val)"
	}
    	label $w.lbl$param -text "$param = $val"
    	pack $w.lbl$param -side top
      }
    }
  } else {
    label $w.lbl -text $chname
    pack $w.lbl -side top
    label $w.spacer1 -text " "
    pack $w.spacer1 -side top
    ;# Create widgets to edit acquire, rate and datatype
    label $w.lbl3 -text "Acquire"
    pack $w.lbl3 -side top
    tk_optionMenu $w.op3 sections($fname,$chname,acquire) 0 1
    pack $w.op3 -side top
    label $w.lbl4 -text "Rate"
    pack $w.lbl4 -side top
    set rates [get_rates $sections($fname,default,datarate)]
    eval "tk_optionMenu $w.op4 sections($fname,$chname,datarate) $rates"
    pack $w.op4 -side top
    label $w.lbl5 -text "Data Type"
    pack $w.lbl5 -side top
    tk_optionMenu $w.op5 sections($fname,$chname,datatype) "short" "float"
    pack $w.op5 -side top
    label $w.spacer2 -text " "
    pack $w.spacer2 -side top
    button $w.b -text "Deactivate" -command "remove_channel"
    pack $w.b -side bottom
  }
}

;# Display disabled channels
proc display_channels file {
  global sections
  global section_names
  set w .f.c

  ;# Cleanup display frame first
  if {[winfo exists $w]} {
        killChildren $w
  }

  ;# Create scrollable list of channels
  scrollbar $w.s -command "$w.l yview"
  listbox $w.l -yscroll "$w.s set" -bg white -selectmode extended
  button $w.b -text "Activate" -command "add_channel"
  label $w.lbl -text "Inactive Channels" -width 50
  pack $w.b -side bottom
  pack $w.lbl -side top
  pack $w.l -side left -fill both -expand 1
  pack $w.s -side left -fill y 

  ;# Insert channel names from the selected ini file
  #parse_ini_file $file
    #foreach key [lsort [array names sections]] {
      #puts  "$key $sections($key)"
    #}
  #puts [lsort [array names sections]]
  set names_off {}
  ;# remove leading slash
  set f [string range $file 1 end]
  foreach i $section_names($f) {
	#puts sections($f,$i,onoff)
	#puts $sections($f,$i,onoff)
  	if {[string compare  $sections($f,$i,onoff) off] == 0} {
		lappend names_off $i
	}
  }
  eval "$w.l insert 0 $names_off"
  #$w.l activate 0
  #bind $w.l <B1-ButtonRelease> {puts [.f.c.l get active]}
}

#puts $files;
;#set oopt [lindex $files 0]


foreach z $files {
  Tree:newitem .f.tf.w /$z -image idir
  parse_ini_file $z
  foreach x $section_names($z) {
    #puts "Section name is $x"
    if {[string compare  $sections($z,$x,onoff) on] == 0} {
      Tree:newitem .f.tf.w /$z/$x -image ifile
    }
  }
}

.f.tf.w bind x <1> {
  set lbl [Tree:labelat %W %x %y]
  Tree:setselection %W $lbl
  #.f.c.l config -text $lbl
  #puts $lbl
  ;# If clicked on file name, then display turned off channel list
  set current_tree_node $lbl
  if { [string last / $lbl] == 0} {
	display_channels $lbl
  } else {
    ;# If clicked on channel name, then display channel parameter edit panel
    display_params $lbl
  }
}
.f.tf.w bind x <Double-1> {
  Tree:open %W [Tree:labelat %W %x %y]
}
update
