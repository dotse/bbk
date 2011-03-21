#!/bin/sh
# Copy all dependencys to tptest5.app/Contents/Frameworks
for DYLIB in $(otool -L tptest5.app/Contents/MacOS/tptest5 | grep dylib | grep -v libSystem | grep -v "@executable_path" | awk '{print $1}'); do 
  cp -v ${DYLIB} tptest5.app/Contents/Frameworks/;
done
# Change dependency path from system to the ones included
for DYLIB in $(otool -L tptest5.app/Contents/MacOS/tptest5 | grep dylib | grep -v libSystem | grep -v "@executable_path" | awk '{print $1}'); do 
  install_name_tool -change ${DYLIB} @executable_path/../Frameworks/$(echo ${DYLIB} | awk 'BEGIN{FS="/";}{print $NF;}') tptest5.app/Contents/MacOS/tptest5;
done
