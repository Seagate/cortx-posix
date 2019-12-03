#!/usr/bin/env python3

import os
import sys
import logging

expected_dirs = 0
expected_files = 0
dname = 'dir.'
fname = 'file.'
dirconflicts = 0
fileconflicts = 0
totfiles = 0
totdirs = 0
LOG_FILENAME = '/tmp/concurrency-{}.log'.format(os.getpid())

logging.basicConfig(filename = LOG_FILENAME, filemode = 'w', format = '%(asctime)s-%(levelname)s: %(message)s', level = logging.DEBUG)

def create(lev, dirs, files):
	#Create a directory tree with the specified levels, no. of directories and files at each level
	global dirconflicts, fileconflicts, totfiles, totdirs, dname, fname

	if lev == 0:
		return 0
	for dir_no in range(dirs):

		name = dname + str(lev) + '.' + str(dir_no)
		try:
			if os.path.isdir(name):
				dirconflicts += 1
				logging.debug('{}: directory exists'.format(client))
			else:
				os.mkdir(name)
				totdirs += 1;

			os.chdir(name)
		except OSError as err:
			logging.critical('{}: {}'.format(client, err))
		except Exception as err:
			logging.critical('{}: {}'.format(client, err))

		create(lev - 1, files, dirs)

		try:
			for file_no in range(files):
				name = fname + str(lev) + '.' + str(file_no)
				if os.path.exists(name):
					fileconflicts += 1
					logging.debug('{}: file exists'.format(client))
				else:
					fd = open(name, 'w+')
					fd.write('client')
					fd.close()
					totfiles += 1

			os.chdir('..')
		except OSError as err:
			logging.critical('{}: {}'.format(client, err))
		except Exception as err:
			logging.critical('{}: {}'.format(client, err))


def total_count(lev, directory, files):
	#Calculating total number of files and directories
	global expected_dirs, expected_files

	dir_list = [ directory**h for h in range(1, lev + 1)]
	expected_dirs = sum(dir_list)
	expected_files = expected_dirs * files


def check_dir(lev, dirs, files):

	logging.debug("{}: Starting check_dir".format(client))
	# Set the directory you want to start from
	rootDir = '.'
	dircnt = 0
	filecnt = 0
	for dirName, subdirList, fileList in os.walk(rootDir):
		if dirName != '.':
			dircnt += 1
		#logging.debug('Found directory: %s' % dirName)
		for fname in fileList:
			#logging.debug('\t%s' % fname)
			filecnt += 1
	logging.debug('{}: Files present = {} and Directories present ={}'.format(client, filecnt, dircnt))

	total_count(lev, dirs, files)

	if filecnt == expected_files:
		logging.debug('{}: All files were created successfully'.format(client))
	else:
		logging.debug('{}: No. of files missing = {}'.format(client, expected_files-filecnt))

	if dircnt == expected_dirs:
		logging.debug('{}: All directories were created successfully'.format(client))
	else:
		logging.debug('{}: No. of directories missing ='.format(client, expected_dirs-dircnt))


def create_dir(lev, dirs, files):

	create(lev, dirs, files)
	logging.debug('{}: Total created files = {} and directories = {}'.format(client, totfiles, totdirs))
	logging.debug('{}: Total failed files = {} and directories = {}'.format(client, fileconflicts, dirconflicts))


###### main ######

#Set Variables

path, client, function_call, dlev, ddir, dfile = sys.argv[1:]
lev = int(dlev)
dirs = int(ddir)
files = int(dfile)
function_dict = {
		'create_dirtree' : create_dir,
		'check_dirtree'  : check_dir,
		}


os.chdir(path)

function_dict[function_call](lev, dirs, files)
