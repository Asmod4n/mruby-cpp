MRuby::Gem::Specification.new('mruby-cpp') do |spec|
  spec.license = 'MPL-2'
  spec.authors = 'Hendrik Beskow'
  spec.summary = 'C++ types that state what mruby does with a value, so the compiler refuses a wrong use'
  spec.export_include_paths << "#{spec.dir}/include" if spec.respond_to?(:export_include_paths)
  spec.add_test_dependency 'mruby-string-ext', core: 'mruby-string-ext'
end
