# This script generates a packaging script based on the files used by the last successful run of the program.
#
# This script should be run from the root directory of a checkout out FPSci repository. 
# Normally this means that you'd envoke it with `python scripts/package/fpsci_packager_generator.py`.
# This script outputs a new version of `scripts/package/fpsci_packager.sh` based on the latest `data-files/log.txt`.
# Both of those can be changed in the parameters below
#
# Once generated, `scripts/package/fpsci_packager.sh` should be run using bash from the root of the repository.
# It will create a `dist/` directory and copy all of the needed files into it.
#
# The intention is that the contents of the `dist/` directory can then be zipped up for distribution.

# Current release being built. Defaults to the git hash. Use "master", "branch name" or "tag name" for other builds.
import subprocess
currentRelease = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).strip().decode()
# currentRelease = "master"

# Name of the packaging script to generate
outputScript = 'scripts/package/fpsci_packager.sh'
outputBatchScript = 'scripts/package/fpsci_packager.bat'

# Path of the log.txt file to use
inputLog = 'data-files/log.txt'

# The list of files that should always be excluded
excludeList = ['log.txt', 'keymap.Any', 'systemconfig.Any']
# The list of files that should be excluded for release builds, but may want to be included for specific experiment distributions
# Use the empty list if you want to distribute an experiment
secondaryExcludeList = ['experimentconfig.Any', 'startupconfig.Any', 'userconfig.Any', 'userstatus.Any']
# secondaryExcludeList = []

# List of paths to search for to merge into the same distribution directory
basePath = ['/data-files/', '/game/', '/common/']

# Distribution path - where the output script will generate the distribution
distPath = 'dist/'

# Automatic configuation
import os
g3dPath = os.environ.get('g3d').replace('\\','/')
runPath = os.getcwd().replace('\\','/')

########## End script configuration ############

from datetime import datetime
now = datetime.now()

import argparse

if __name__ == '__main__':
    # Command line options
    parser = argparse.ArgumentParser(description='Create a packager script for First Person Science.')
    parser.add_argument('--release', help='Name of release for README.txt i.e. v20.08.01')
    parser.add_argument('--expbuild', help='Create an experiment build with provided name')
    args = parser.parse_args()
    print(args)
    if args.expbuild is not None:
        secondaryExcludeList = []
        print('Creating experiment build <' + distPath + '>')
    if args.release is not None:
        currentRelease = args.release
        print('Creating release build <' + currentRelease + '>')

    # Get the current set of lines in the script
    lineSet = set(open(outputScript))
    for line in sorted(lineSet):
        if line.startswith('#'):
            lineSet.remove(line)
        if line.startswith('mv '):
            lineSet.remove(line)

    # Hard coded files to copy
    lineSet.add('mkdir -p ' + distPath + '\n')
    lineSet.add('cp data-files/g3d-license.txt '  + distPath + '\n')
    lineSet.add('cp LICENSE.txt '  + distPath + '\n')
    lineSet.add('cp README.txt '  + distPath + '\n')
    # For some reason these files aren't captured by G3D as used
    lineSet.add('mkdir -p '+ distPath + 'shader/DefaultRenderer/\n')
    lineSet.add('cp $g3d/G3D10/data-files/shader/DefaultRenderer/DefaultRenderer_OIT_writePixel.glsl '+ distPath + 'shader/DefaultRenderer/\n')
    lineSet.add('mkdir -p '+ distPath + 'shader/UniversalSurface/\n')
    lineSet.add('cp $g3d/G3D10/data-files/shader/UniversalSurface/UniversalSurface_depthPeel.pix '+ distPath + 'shader/UniversalSurface/\n')

    # Set up copy of .exe
    lineSet.add('cp vs/Build/FirstPersonScience-x64-Release/FirstPersonScience.exe ' + distPath + '\n')

    # The log file which lists the files used
    try:
        log = open(inputLog)
    except FileNotFoundError:
        print('No %s found, continuing without it.'%(inputLog))
    else:
        with log:
            print('Adding files from %s to %s.'%(inputLog, outputScript))
            # main loop, need to ignore lines before the file list begins
            beforeFiles = True
            for line in log.readlines():
                # In the file list, add this file to what we want to copy
                if not beforeFiles and line != '\n':
                    filename = line.strip()
                    basename = filename.split('/')[-1]

                    # Exclude the lists of files we don't want to package
                    if basename in excludeList or basename in secondaryExcludeList:
                        continue
                    
                    # Absolute path given
                    if filename[1:].startswith(':/'):
                        for path in basePath:
                            dest = filename.find(path)
                            if dest > 0:
                                dest += len(path)
                                additionalPath = filename[dest:-len(basename)]
                                # if this file is inside a compressed structure, skip copying it
                                if additionalPath.find('.pk3') > 0 or additionalPath.find('.zip') > 0:
                                    break

                                # Make paths generic
                                filename = filename.replace(g3dPath, "$g3d").replace(runPath, ".")

                                # Write commands to file
                                lineSet.add('mkdir -p ' + distPath + additionalPath + '\n')
                                lineSet.add('cp ' + filename + ' ' + distPath + additionalPath + '\n')

                    # file is in the PATH, need to find it
                    else:
                        # Use the `which` version if $g3d is the wrong path
                        # lineSet.add('cp `which ' + filename + '` ' + distPath + '\n')
                        lineSet.add('cp $g3d/G3D10/build/bin/' + filename + ' ' + distPath + '\n')

                # Find where the filenames start
                if line.startswith('    ###    Files Used    ###'):
                    beforeFiles = False

    # Write the set of commands in sorted order
    with open(outputScript, 'w') as packageScript:
        print('Writing updated bash script to ' + outputScript + '.')
        # Script header and timestamp
        packageScript.writelines(['#!/bin/bash\n', '# Autogenerated ' + now.strftime('%Y %m %d %H:%M:%S\n')])
        for line in sorted(lineSet, reverse=True):
            packageScript.write(line)
        if args.expbuild is not None:
            packageScript.write('mv ' + distPath + ' ' + args.expbuild + '\n')

    # Write the set of commands in sorted order as a batch script
    with open(outputBatchScript, 'w') as packageScript:
        print('Writing updated batch script to ' + outputBatchScript + '.')
        # Script timestamp
        packageScript.writelines(['REM Autogenerated ' + now.strftime('%Y %m %d %H:%M:%S\n')])
        for line in sorted(lineSet, reverse=True):
            packageScript.write(line.replace('/', '\\').replace('mkdir -p ', 'mkdir ').replace('cp ', 'copy ').replace('$g3d', '%g3d%').replace('mv ', 'rename '))

    # Write readme
    readmeString = "Welcome to First Person Science! We're glad you're joining us in studying first person aiming.\n\n"
    readmeString += "This distribution represents the %s build of FPSci generated on %s.\n\n"%(currentRelease, now.strftime('%B %d, %Y'))
    readmeString += "You can find documentation on how to create and customize your experiments here: https://github.com/NVlabs/abstract-fps/tree/%s/docs\n\n"%(currentRelease)
    readmeString += "In order to use this build, you may need to install the x64 version (and possibly the x86 version) of the Microsoft Visual C++ Redistributable found at this link available here: https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads\n\n"
    readmeString += "If you're interested, the source for this version is available here: https://github.com/NVlabs/abstract-fps/tree/%s\n"%(currentRelease)
    with open('README.txt', 'w') as readmeFile:
        print('Writing generated readme to README.txt.')
        readmeFile.write(readmeString)

