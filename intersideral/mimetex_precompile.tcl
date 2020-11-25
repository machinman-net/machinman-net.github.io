#! /usr/bin/tclsh

# Copyright Emmanuel Azencot mer, sam. 02 oct. 2010 11:15:44 CEST 
#
# LICENCE :
# This software is under the GNU GPL license 2.
# You should have received a copy of the licence 
# 
#                          Version 2, June 1991
#      Copyright (C) 1989, 1991 Free Software Foundation, Inc.
#      675 Mass Ave, Cambridge, MA 02139, USA
#      
# GPL license along with this software os. if you did not you can find it
# at http://www.gnu.org/.
#
# DESCRIPTION :
# Cet outil precompile les expressions mathematiques LaTex contenues dans une page avec mimetex et produit les images et liens Ad Hoc.
# Il permet d'utiliser mimetex sans serveur WEB au moment de la conception des pages. 
# Et ne requiert pas la possibilité de lancer des binaires comme mimetex sur votre hebergement WEB, 
# certains hebergeurs gratuits ne fournissant pas cette possibilité.
#
# La classe "mimetex" est ajoutee a toutes les balises images concernees pour permettre la mise en page CSS pour l'ensemble des images.
# 
# EXEMPLES :
# <img mimetex="D=D_a+D_c"> -> <img mimetex="D=D_a+D_c" src="calc_duree_trip//mimetex_img_0.gif" alt="D=D_a+D_c" class="mimetex">
# <img mimetex="D=D_a+D_c" src="calc_duree_trip//mimetex_img_3.gif"> -> <img mimetex="D=D_a+D_c" alt="D=D_a+D_c" class="mimetex">
# <img mimetex="D=D_a+D_c" src="calc_duree_trip//mimetex_img_3.gif" alt="D=Da+Dc"> -> <img mimetex="D=D_a+D_c"  alt="D=Da+Dc" class="mimetex">
# <img src="http://foo.bar/cgi-bin//mimetex.cgi?calc_duree_trip//mimetex_img_3.gif"> -> <img mimetex="D=D_a+D_c" alt="D=D_a+D_c" class="mimetex">
# <img src="http://foo.bar/cgi-bin//mimetex.cgi?calc_duree_trip//mimetex_img_3.gif" alt="D=Da+Dc"> -> <img mimetex="D=D_a+D_c" alt="D=Da+Dc" class="mimetex">
#
# USAGE : 
#   ./mimetex_precompile.tcl -d <img_dir> -o <output.html> [-i <input.html>]
#
# INSTALLATION (Linux):
#   - requiert : TCL (http://www.tcl.tk/)
#       Standart install packages Group: System/Libraries
#   - requiert : tDom http://github.com/makr/tdom http://web.archive.org/web/20080805035611/http://www.tdom.org/ (or http://www.tdom.org  when it will be fixed)
#       RPM bin package available for most distros.
#   - requiert : mimetex http://www.forkosh.com/mimetex.html
#       RPM bin package available for most distros.
#   Change variable MIMETEX_BIN according to your install, if necessary
# BUG :
#  En raison du passage par un parseur XML, la sortie a toujours le codage des charactères en UTF-8. Vous pouvez utiliser la balise 
#    <meta content="text/html; charset=UTF-8" http-equiv="Content-Type">
# dans le header de la page pour un affichage correct des charactères spéciaux

set MIMETEX_BIN "";
if { $MIMETEX_BIN == "" && ![catch {exec /var/www/cgi-bin/mimetex.cgi}] } {
   set MIMETEX_BIN "/var/www/cgi-bin/mimetex.cgi"
}
if { $MIMETEX_BIN == "" && ![catch {exec ./mimetex.cgi}] } {
   set MIMETEX_BIN "./mimetex.cgi"
}
if { $MIMETEX_BIN == "" && ![catch {exec ../mimetex/mimetex.cgi}] } {
   set MIMETEX_BIN "../mimetex/mimetex.cgi"
}
if { $MIMETEX_BIN == "" && ![catch {exec mimetex/mimetex.cgi}] } {
   set MIMETEX_BIN "mimetex/mimetex.cgi"
}
if { $MIMETEX_BIN == "" && ![catch {exec mimetex.cgi}] } {
   set MIMETEX_BIN "mimetex.cgi"
}
if { $MIMETEX_BIN == "" } {
   puts stderr "$argv0: Could not find mimetex executable, please install or set MIMETEX_BIN variable with its path";
   exit 1;
}
puts stderr "mimetex as $MIMETEX_BIN"

package require tdom;

set USAGE "USAGE: $argv0 \[-a\] -d <img_dir> \[-u <update.html>\] \[-o <output.html>\] \[-i <input.html>\]"
set HELP "\
  -d : Images destination directory\n\
  -u : update file\n\
  -o : output file\n\
  -i : optional input file. stdin is use if option not present\n\
  -a : force \"alt\" attribute to be set to Latex expression"

# Parse command line arguments
if { $argc == 1 && [lindex $argv 0] == "-h" } { 
  puts stderr "$USAGE"; puts stderr "$HELP", exit 1; 
}
if { $argc < 4 || $argc > 7 } {
   puts stderr "$USAGE";
   exit 1;
}
set opt_alt 0;
set opt_i 0;
while { $opt_i < $argc } {
   switch -- [lindex $argv $opt_i] {
     "-d" { incr opt_i; set opt_dir   [lindex $argv $opt_i]; }
     "-o" { incr opt_i; set opt_ofile [lindex $argv $opt_i]; }
     "-i" { incr opt_i; set opt_ifile [lindex $argv $opt_i]; }
     "-u" { incr opt_i; set opt_ifile [lindex $argv $opt_i]; set opt_ofile [lindex $argv $opt_i]; }
     "-a" { set opt_alt 1; }
     default  {
       puts stderr "$argv0 : unknow option [lindex $argv $opt_i]";
       exit 1;
     }
   }
   incr opt_i;
}
if { ![info exist opt_ofile] } {
   puts stderr "$argv0 : no output file specified";
   exit 1;
}
if { ![info exist opt_dir] } {
   puts stderr "$argv0 : no image dir specified";
   exit 1;
}
# Setup dest directory for images, remove trailing "/"
if { [string length $opt_dir] != 1 } { set opt_dir [string trimright $opt_dir "/"]; }
puts stderr "opt_dir = $opt_dir"
if { [catch {file mkdir $opt_dir} msg] } {
   puts stderr "$argv0 : $msg";
   exit 1;
}
# Open and parse HTML source doc
if { [info exists opt_ifile] } {
   set fl [open $opt_ifile];
   set doc [dom parse -html -keepEmpties [read $fl]];
   close $fl;
} else {
   set doc [dom parse -html -keepEmpties [read stdin]];
}
set html [$doc getElementsByTagName html];

# Biuld image from latex expression (in var mimetex), and setup img attributes
proc update_img { img mimetex img_num } {
   global MIMETEX_BIN;
   global opt_dir;
   global opt_alt;
   # puts stderr "$mimetex / $img_num"
   # Build image with mimetex/var/www/cgi-bin/mimetex.cgi
   exec $MIMETEX_BIN -d $mimetex >$opt_dir/mimetex_img_$img_num.gif;
   # Removes image size attributes
   catch {$img removeAttribute width};
   catch {$img removeAttribute height};
   # Change link to new image
   $img setAttribute src "$opt_dir/mimetex_img_$img_num.gif";
   # Set up alt attribute with LaTex expression, if empty or if specified with "-a" option
   if { [catch {$img getAttribute "alt"}] || $opt_alt == 1 } {
      $img setAttribute alt "$mimetex";
   }
   # Set up mimetex classe for CSS formating
   $img setAttribute class "mimetex";
   incr img_num;
   return $img_num;
}

set img_num 0
# For all images in file
foreach img [$html getElementsByTagName img] {
   # Image have attribute "mimetex" ?
   if { ![catch {$img getAttribute "mimetex"} mimetex] } {
      # Build image with mimetex
      set img_num [update_img $img $mimetex $img_num]
      continue;
   }
   # Image have attribute "src" matching ".*/mimetex.cgi?.*" ?
   if { ![catch {$img getAttribute "src"} src] } { 
      set mimetex [string last {/mimetex.cgi?} $src ]
      if { $mimetex >= 0 } {
         incr mimetex [string length {/mimetex.cgi?} ]
         set mimetex [string range $src $mimetex end]
         puts stderr "img with \nsrc = $src\nalt = $alt\n mimetex : $mimetex"
         set img_num [update_img $img $mimetex $img_num]
         continue;
      }
   }
}
set fl [open $opt_ofile "w"];
$doc asHTML -channel $fl;
close $fl;


