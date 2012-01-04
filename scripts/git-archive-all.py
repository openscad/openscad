#! /usr/bin/env python

import sys
from os import path, chdir
from subprocess import Popen, PIPE
from sys import argv, stdout
from fnmatch import fnmatch


class GitArchiver(object):
    """
    GitArchiver
    
    Scan a git repository and export all tracked files, and submodules.
    Checks for .gitattributes files in each directory and uses 'export-ignore'
    pattern entries for ignore files in the archive.
    
    Automatically detects output format extension: zip, tar, bz2, or gz
    """
    
    def __init__(self, prefix='', verbose=False, exclude=True, extra=[]):
        self.prefix    = prefix
        self.verbose   = verbose
        self.exclude   = exclude
        self.extra     = extra
        
        self._excludes = []


    def create(self, output_file):
        """
        create(str output_file) -> None
        
        Creates the archive, written to the given output_file
        Filetype may be one of:  gz, zip, bz2, tar
        """
        #
        # determine the format
        #
        _, _, format = output_file.rpartition(".")

        if format.lower() == 'zip':
            from zipfile import ZipFile, ZIP_DEFLATED
            output_archive = ZipFile(path.abspath(output_file), 'w')
            add = lambda name, arcname: output_archive.write(name, self.prefix + arcname, ZIP_DEFLATED)
    
        elif format.lower() in ['tar', 'bz2', 'gz']:
            import tarfile
            t_mode = ('w:%s' % format) if format != 'tar' else 'w'
               
            output_archive = tarfile.open(path.abspath(output_file), t_mode)
            add = lambda name, arcname: output_archive.add(name, self.prefix + arcname)
        else:
            raise RuntimeError("Unknown format: '%s'" % format)
        
        #
        # compress
        #
        
        # extra files first (we may change folder later)
        for name in self.extra:
            if self.verbose: 
                toPath = '=> %s%s' % (self.prefix, name) if self.prefix else ""
                print 'Compressing %s %s ...' % (name, toPath)
            add(name, name)
        
        self._excludes = []
        
        for name, arcname in self.listFiles(path.abspath('')):
            if self.verbose: 
                toPath = '=> %s%s' % (self.prefix, arcname) if self.prefix else ""
                print 'Compressing %s %s ...' % (arcname, toPath)
            add(name, arcname)
      
        output_archive.close()
        
            
    def listFiles(self, git_repositary_path, baselevel=''):
        """
        listFiles(str git_repository_path, str baselevel='') -> iterator
        
        An iterator method that yields a tuple(filepath, fullpath)
        for each file that should be included in the archive.
        Skips those that match the exclusion patterns found in
        any discovered .gitattributes files along the way.
        
        Recurses into submodules as well.
        """
        for filepath in self.runShell('git ls-files --cached --full-name --no-empty-directory'):
            fullpath = path.join(baselevel, filepath)
            filename = path.basename(filepath)
            
            if self.exclude and filename == '.gitattributes':
                self._excludes = []
                fh = open(filepath, 'r')
                for line in fh:
                    if not line: break
                    tokens = line.strip().split()
                    if 'export-ignore' in tokens[1:]:
                        self._excludes.append(tokens[0])
                fh.close()
    
            if not filename.startswith('.git') and not path.isdir(filepath):
            
                # check the patterns first
                ignore = False
                for pattern in self._excludes:
                    if fnmatch(fullpath, pattern) or fnmatch(filename, pattern):
                        if self.verbose: print 'Exclude pattern matched (%s): %s' % (pattern, fullpath)
                        ignore = True
                        break
                if ignore:
                    continue
                
                # baselevel is needed to tell the arhiver where it have to extract file
                yield filepath, fullpath
        
        # get paths for every submodule
        for submodule in self.runShell("git submodule --quiet foreach 'pwd'"):
            chdir(submodule)
            # in order to get output path we need to exclude repository path from the submodule path
            submodule = submodule[len(git_repositary_path)+1:]
            # recursion allows us to process repositories with more than one level of submodules
            for git_file in self.listFiles(git_repositary_path, submodule):
                yield git_file
    

    
    @staticmethod
    def runShell(cmd):
        return Popen(cmd, shell=True, stdout=PIPE).stdout.read().splitlines()
    
    
    
if __name__ == "__main__":
    from optparse import OptionParser

    parser = OptionParser(usage="usage: %prog [-v] [--prefix PREFIX] [--no-exclude] OUTPUT_FILE", version="%prog 1.0")
    
    parser.add_option('--prefix', type='string', dest='prefix', 
                      default='', help="prepend PREFIX to each filename in the archive")
                        
    parser.add_option('-v', '--verbose', action='store_true', dest='verbose', help='enable verbose mode')

    parser.add_option('--no-exclude', action='store_false', dest='exclude', 
                      default=True, help="Dont read .gitattributes files for patterns containing export-ignore attrib")
                        
    parser.add_option('--extra', action='append', dest='extra', default=[], 
                      help="Any additional files to include in the archive.")

    options, args = parser.parse_args()
    
    if len(args) != 1:
        parser.error('You must specify exactly one output file')
    
    outFile = args[0]
    
    if path.isdir(outFile):
        parser.error('You cannot use directory as output')
            
    archiver = GitArchiver(options.prefix, 
                           options.verbose, 
                           options.exclude,
                           options.extra)
    
    try:
        archiver.create(outFile)
    except Exception, e:
        parser.exit(2, "%s\n" % e)
    
    sys.exit(0)
