MRuby::Gem::Specification.new('mruby-cpp') do |spec|
  spec.license = 'MPL-2.0'
  spec.authors = 'Hendrik Beskow'
  spec.summary = 'C++ types that state what mruby does with a value, so the compiler refuses a wrong use'
  spec.export_include_paths << "#{spec.dir}/include"
  spec.add_test_dependency 'mruby-string-ext', core: 'mruby-string-ext'
  spec.add_test_dependency 'mruby-metaprog', core: 'mruby-metaprog'
  years = { '98' => 1998, '03' => 2003, '0x' => 2011, '11' => 2011, '1y' => 2014, '14' => 2014, '1z' => 2017, '17' => 2017,
            '2a' => 2020, '20' => 2020, '2b' => 2023, '23' => 2023, '2c' => 2026, '26' => 2026 }
  named = spec.build.cxx.flags.flatten.map(&:to_s).grep(/\A-std=(?:c|gnu)\+\+\w\w\z/).last
  spec.cxx.flags << '-std=c++23' if named.nil? || years.fetch(named[-2..], 0) < 2023
end
