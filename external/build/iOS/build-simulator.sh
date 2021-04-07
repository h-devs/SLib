$(dirname $0)/../macOS/build.sh
THIS_PATH=$(dirname $0)
SIMULATOR_VERSION=`xcodebuild -showsdks | grep iphonesimulator`
xcodebuild -project "${THIS_PATH}/External.xcodeproj" -scheme External -configuration Release -sdk iphonesimulator${SIMULATOR_VERSION##*iphonesimulator} -derivedDataPath "${THIS_PATH}/build"