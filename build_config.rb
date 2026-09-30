MRuby::Build.new do |conf|
  conf.toolchain :gcc
  conf.enable_debug
  conf.enable_test
  conf.gem File.expand_path(__dir__)
end
