set $dir=/tmp
set $nfiles=1000
set $meandirwidth=1
set $nthreads=1

set mode quit firstdone

define fileset name=bigfileset2,path=$dir,entries=$nfiles,dirwidth=$meandirwidth,prealloc

define process name=filereader,instances=1
{
  thread name=filereaderthread,memsize=10m,instances=$nthreads
  {
    flowop createfile name=createfile1,filesetname=bigfileset2,fd=1
    flowop openfile name=openfile1,filesetname=bigfileset2,fd=1
    flowop closefile name=closefile2,fd=1
    flowop deletefile name=deletefile1,filesetname=bigfileset2
    flowop statfile name=statfile1,filesetname=bigfileset2
  }
}

echo  "File-server Version 3.0 personality successfully loaded"

run 60
