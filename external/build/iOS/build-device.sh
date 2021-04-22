$(dirname $0)/../macOS/build.sh
THIS_PATH=$(dirname $0)
xcodebuild -project "${THIS_PATH}/External.xcodeproj" -scheme External -configuration Release -derivedDataPath "${THIS_PATH}/build"