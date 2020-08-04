set $dir=/tmp
set $nfiles=1000
set $meandirwidth=1
set $nthreads=50
set $iosize=1m
set $meanappendsize=4k

define fileset name=bigfileset1,path=$dir,size=$filesize,entries=$nfiles,dirwidth=$meandirwidth,prealloc=80

define process name=filereader,instances=1
{
  thread name=filereaderthread,memsize=10m,instances=$nthreads
  {
    flowop openfile name=openfile3,filesetname=bigfileset1,fd=1
    flowop appendfilerand name=appendfilerand3,iosize=$meanappendsize,fd=1
    flowop closefile name=closefile3,fd=1
    flowop openfile name=openfile4,filesetname=bigfileset1,fd=1
    flowop readwholefile name=readfile4,fd=1,iosize=$iosize
    flowop closefile name=closefile4,fd=1
  }
}

echo  "Varmail Version 3.0 personality successfully loaded"

run 60
