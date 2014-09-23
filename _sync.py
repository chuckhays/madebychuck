#!/usr/bin/python
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('username', action="store", help='ssh username for rsync')

args = parser.parse_args()

os.system('rsync --delete -a -v -e "ssh" _site/ ' + args.username + '@madebychuck.com:~/madebychuck.com/')
