set $dir=/tmp
set $nfiles=1000
set $meandirwidth=5
set $nthreads=1

set mode quit firstdone

define fileset name=bigfileset3,path=$dir,size=0,leafdirs=$nfiles,dirwidth=$meandirwidth
define fileset name=bigfileset4,path=$dir,size=0,entries=$nfiles,dirwidth=$meandirwidth,prealloc
define fileset name=bigfileset5,path=$dir,size=0,leafdirs=$nfiles,dirwidth=$meandirwidth,prealloc

define process name=dirmake,instances=1
{
  thread name=dirmaker,memsize=1m,instances=$nthreads
  {
    flowop makedir name=mkdir1,filesetname=bigfileset3
  }
  thread name=dirlister,memsize=1m,instances=$nthreads
  {
    flowop listdir name=open1,filesetname=bigfileset4
  }
  thread name=removedirectory,memsize=1m,instances=$nthreads
  {
    flowop removedir name=dirremover,filesetname=bigfileset5
  }

}

echo  "MakeDirs Version 1.0 personality successfully loaded"
run 60 
