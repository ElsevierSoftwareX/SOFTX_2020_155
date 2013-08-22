#!/usr/bin/env bash
#
#  Script to remove old trend files for IIR filter channels no longer recorded
#   - required after RCG 2.7
#
#   Keith Thorne August, 2013
#
# - must be run from 'minute_raw' directory 
dry_run=1;
if [ "$1" = "move" ]; then
   dry_run=0;
fi

#
# script to delete old filter trends
#
# get list of _SW2R files
ls */*_SW2R > SW2R_list.txt
#
# count them

# strip to filters
cat SW2R_list.txt | sed 's:^[a-f0-9]*/:\*/:' | sed 's:_SW2R$:_:' | sort > filter_list.txt
nfilt=$(wc filter_list.txt | awk '{print $1}')
echo 'Number of filters:' $nfilt

suffi=("SW1" "SW1R" "SW1S" "SW2" "SW2R" "SW2S" "RSET" "OUTMON")
for filsuf in "${suffi[@]}"; do
   echo 'Check '$filsuf
   cat filter_list.txt | sed "s:_$:_${filsuf}:" > "${filsuf}"_wild.txt
   ls `cat "${filsuf}"_wild.txt` > "${filsuf}"_filelist.txt
   numfound=$( wc "${filsuf}"_filelist.txt | awk '{print $1}')
   if [ "$numfound" -ne "$nfilt" ]; then
     echo 'ERROR '$filsuf 'has wrong number of files: ' $numfound	
   else
     echo 'OK - found correct number of files'
   fi
done

dumpdir="oldtrd"
mkdir -p ${dumpdir}
for filsuf in "${suffi[@]}"; do
   nummove=$( wc "${filsuf}"_filelist.txt | awk '{print $1}')
   echo 'move ' $nummove ${filsuf}' trend files to '$dumpdir
   for i in `cat "${filsuf}"_filelist.txt`; do
      if [ $dry_run = 0 ]; then
         mv "$i" ${dumpdir}
      else
         ls -l "$i" 
      fi
   done
done

