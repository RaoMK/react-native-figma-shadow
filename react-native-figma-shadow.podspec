require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

Pod::Spec.new do |s|
  s.name         = "react-native-figma-shadow"
  s.version      = package["version"]
  s.summary      = package["description"]
  s.homepage     = package["homepage"]
  s.license      = package["license"]
  s.authors      = package["author"]

  s.platforms    = { :ios => "13.4" }
  s.source       = { :git => "https://github.com/RaoMK/react-native-figma-shadow.git", :tag => "#{s.version}" }

  # Only .cpp goes through source_files. Our headers (Color.h, Types.h, ...) share
  # basenames with React Native's; keeping them out of source_files stops
  # CocoaPods flattening them into a shared Headers dir. They are reached only as
  # "figmashadow/<name>.h" via the single search path below, or as siblings from
  # our own .cpp files.
  s.source_files = "ios/**/*.{h,m,mm}", "cpp/figmashadow/**/*.cpp"
  s.preserve_paths = "cpp/figmashadow/**/*.h"

  s.pod_target_xcconfig = {
    "CLANG_CXX_LANGUAGE_STANDARD" => "c++17",
    "HEADER_SEARCH_PATHS" => "\"$(PODS_TARGET_SRCROOT)/cpp\""
  }

  install_modules_dependencies(s)
end
