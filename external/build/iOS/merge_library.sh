SRC_PATH=$(dirname $0)/build/lib
LIB_PATH=$(dirname $0)/../../lib/iOS
mkdir -p $LIB_PATH
LIB_NAME=freetype
lipo -create $SRC_PATH/iphoneos/lib$LIB_NAME.a $SRC_PATH/iphonesimulator/lib$LIB_NAME.a -o $LIB_PATH/lib$LIB_NAME.a
LIB_NAME=opus
lipo -create $SRC_PATH/iphoneos/lib$LIB_NAME.a $SRC_PATH/iphonesimulator/lib$LIB_NAME.a -o $LIB_PATH/lib$LIB_NAME.a
LIB_NAME=vpx
lipo -create $SRC_PATH/iphoneos/lib$LIB_NAME.a $SRC_PATH/iphonesimulator/lib$LIB_NAME.a -o $LIB_PATH/lib$LIB_NAME.a
LIB_NAME=sqlite3
lipo -create $SRC_PATH/iphoneos/lib$LIB_NAME.a $SRC_PATH/iphonesimulator/lib$LIB_NAME.a -o $LIB_PATH/lib$LIB_NAME.a
LIB_NAME=hiredis
lipo -create $SRC_PATH/iphoneos/lib$LIB_NAME.a $SRC_PATH/iphonesimulator/lib$LIB_NAME.a -o $LIB_PATH/lib$LIB_NAME.a
LIB_NAME=zxing
lipo -create $SRC_PATH/iphoneos/lib$LIB_NAME.a $SRC_PATH/iphonesimulator/lib$LIB_NAME.a -o $LIB_PATH/lib$LIB_NAME.a
