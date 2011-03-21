#!/bin/sh
echo -n "Creating universal binary... "
mkdir -p tptest5.app/Contents tptest5.app/Contents/MacOS tptest5.app/Contents/Frameworks tptest5.app/Contents/Resources tptest5.app/Contents/PlugIns
cp tptest5-intel.app/Contents/Frameworks/* tptest5.app/Contents/Frameworks/ && rm -f tptest5.app/Contents/Frameworks/libcurl.4.dylib
strip tptest5-intel.app/Contents/MacOS/tptest5
strip tptest5-ppc.app/Contents/MacOS/tptest5
lipo -create tptest5-intel.app/Contents/Frameworks/libcurl.4.dylib tptest5-ppc.app/Contents/Frameworks/libcurl.4.dylib -output tptest5.app/Contents/Frameworks/libcurl.4.dylib
lipo -create tptest5-intel.app/Contents/Frameworks/libgcc_s.1.dylib tptest5-ppc.app/Contents/Frameworks/libgcc_s.1.dylib -output tptest5.app/Contents/Frameworks/libgcc_s.1.dylib
lipo -create tptest5-intel.app/Contents/Frameworks/libiconv.2.dylib tptest5-ppc.app/Contents/Frameworks/libiconv.2.dylib -output tptest5.app/Contents/Frameworks/libiconv.2.dylib
lipo -create tptest5-intel.app/Contents/Frameworks/libz.1.dylib tptest5-ppc.app/Contents/Frameworks/libz.1.dylib -output tptest5.app/Contents/Frameworks/libz.1.dylib
lipo -create tptest5-intel.app/Contents/Frameworks/libstdc++.6.dylib tptest5-ppc.app/Contents/Frameworks/libstdc++.6.dylib -output tptest5.app/Contents/Frameworks/libstdc++.6.dylib
lipo -create tptest5-intel.app/Contents/MacOS/tptest5 tptest5-ppc.app/Contents/MacOS/tptest5 -output tptest5.app/Contents/MacOS/tptest5
cp tptest5-intel.app/Contents/Resources/help.html tptest5.app/Contents/Resources/help.html
cp tptest5-intel.app/Contents/Resources/start.jpg tptest5.app/Contents/Resources/start.jpg
cp tptest5-intel.app/Contents/Resources/skapa-felrapport.jpg tptest5.app/Contents/Resources/skapa-felrapport.jpg
cp tptest5-intel.app/Contents/Resources/tptest-result-list.jpg tptest5.app/Contents/Resources/tptest-result-list.jpg
cp tptest5-intel.app/Contents/Resources/trend-colorpoint.jpg tptest5.app/Contents/Resources/trend-colorpoint.jpg
tar zcf tptest5-univ.tar.gz tptest5.app
echo "Done"
