#!/bin/bash
windows=false

function find_visual_studio_directory()
{
  env | grep "OS=Windows" > /dev/null && windows=true

  if [ "${windows}" = true ]; then
    wd=$(pwd)
    cd "C:/Program Files (x86)/Microsoft Visual Studio/Installer/" || exit
    find "vswhere.exe" -maxdepth 0 &> /dev/null || (cd "${wd}" && download_vswhere)

    VSShortVer=$(./vswhere.exe | grep "catalog_productLine: Dev17")
    VSShortVer="${VSShortVer:24}"

    VSVer=$(./vswhere.exe | grep "catalog_productLineVersion:")
    VSVer="${VSVer:28}"

    cd "${wd}" || exit
  fi

  return
}

function process_files()
{
  # Cleanup the directory
  rm -rf "CMake*" "Makefile" "*.cmake" &> /dev/null  # Remove all CMake files and Makefiles
  find . -name '*.cmake' -delete &> /dev/null && find . -type d -name "CMakeFiles" -exec rm -rf {} \; &> /dev/null # Remove remaining CMake files
  find Framework/ -name "Makefile" -exec rm -rf {} \; &> /dev/null && find Framework/ -name "*.h" -exec rm -rf {} \; &> /dev/null # Remove all headers
  find Framework/ -name "*.pc" -exec rm -rf {} \; &> /dev/null && find . -type d -empty -exec rmdir {} \; &> /dev/null # Remove all PCs and dirs
  find . -type d -name "docs" -exec rm -rf {} \; &> /dev/null && find . -type d -empty -exec rmdir {} \; &> /dev/null # Remove all docs folders
  find Framework/ -name "*.tcl" -exec rm -rf {} \; &> /dev/null && find . -name "*.txt" -exec rm -rf {} \; &> /dev/null  # Remove all PCs and dirs
  find . -name "*.vcxproj*" -exec rm -rf {} \; &> /dev/null # Delete all vcproj and filter files since they aren't needed
  find . -name "*.vcxproj" -exec rm -rf {} \; &> /dev/null # Delete all vcproj files
  find . -name "x64" -type d -exec rm -rf "{}" \; &> /dev/null # Delete all x64 folders, that contain logs
  find . -name "*.dir" -type d -exec rm -rf "{}" \; &> /dev/null # Delete all object file folders
  find . -name "*.sln" -exec rm -rf {} \; &> /dev/null # Delete all VS solution files
  cd .. || exit
}

function help()
{
  echo "The following script helps you export your project for production"
  echo "It's recommended that you don't run this script alone and use the UVKBuildTool"
  echo "Commands:"
  echo "  --help/help/-H/-h - displays this help message"
  echo "  <anything else> - would treat the argument as the name of a project and compile"
}

if [ "$1" = "" ]; then # If the first argument is empty throw an error and show help
  echo -e "\x1B[31mError: You cannot run the script without an additional argument!\033[0m"
  help
  exit
elif [ "$1" == "help" ] || [ "$1" == "-help" ] || [ "$1" == "--help" ] || [ "$1" == "-H" ] || [ "$1" == "-h" ]; then # shows the help message
  help
  exit
fi

function cmake_i()
{
  cmake "$@" || exit
}

jobs=$(grep -c processor /proc/cpuinfo)
cd Exported/ || exit

find_visual_studio_directory

if [ "$3" == "--system-wide" ]; then
  # This sets all options created with "option" in cmake to "ON". Build variant options are not affected because they are set using rules
  sed -i 's/" OFF)/" ON)/g' ../CMakeLists.txt
fi

if [ "${windows}" == true ]; then
  cmake_i .. -G "Visual Studio ${VSShortVer} ${VSVer}" -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_INSTALL_PREFIX="$2" "${@:4}" || exit
  MSBuild.exe "$1".sln -property:Configuration=Release -property:Platform=x64 -property:maxCpuCount="${jobs}" || exit
  cp Release/"$1".exe . || exit
  cp Release/UntitledImGuiFramework.dll . || exit
else
  cmake_i .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_INSTALL_PREFIX="$2" "${@:4}" || exit
  make -j "${jobs}" || exit
fi

cmake --install . --prefix="$2" || exit

process_files "$1"
mv "CMakeLists.txt.old" "CMakeLists.txt"

echo -e "\x1B[32m--------------------------------------------------------------------------------\033[0m"
echo -e "\x1B[32mBuild Done!\033[0m"
