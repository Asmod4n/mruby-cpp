# C++ that reads the bytes of a Ruby object needs them to stay where they
# are. mruby checks the frozen flag before it changes or moves the buffer
# of a String, an Array or a Hash, so an object that is frozen while a
# mruby::frozen exists holds its buffer in place.
assert('mruby::frozen freezes an object while it exists') do
  text = 'abc'
  assert_true MrubyCppTest.frozen_while_held?(text)
  assert_false text.frozen?
  assert_true MrubyCppTest.frozen_while_held?([1, 2])
  frozen = 'abc'.freeze
  assert_true MrubyCppTest.frozen_while_held?(frozen)
  assert_true frozen.frozen?
  assert_false MrubyCppTest.frozen_while_held?(1)
end

# A change while the object is frozen raises, and the end of the scope
# thaws it also when the scope ends with that raise.
assert('mruby::frozen refuses a change, and thaws on a raise') do
  text = 'abc'
  assert_raise(FrozenError) { MrubyCppTest.append_while_frozen(text) }
  assert_false text.frozen?
  assert_equal 'abc', text
end

assert('mruby::frozen stays frozen until the outer one ends') do
  text = 'abc'
  assert_true MrubyCppTest.frozen_after_inner?(text)
  assert_false text.frozen?
end

# A value that mruby made in the arena lives until the mruby call returns,
# and a freeze lasts for one scope. A copy, a move or new would carry either
# past that point.
assert('mruby::frozen and mruby::automatic cannot leave their scope') do
  assert_true MrubyCppTest.scope_bound?
end

# A value that C++ keeps beyond a call is held by a root.
assert('mruby::root keeps a value through a full collection') do
  assert_true MrubyCppTest.root_keeps_value?
end

# A destructor may not run mruby, so releasing a root only lists it. The
# next root unregisters it, and the collector then frees the object.
assert('a released mruby::root is collected after the next root') do
  assert_true MrubyCppTest.released_root_is_collected?
end

# mrb_close frees every object. A root that outlives its state must not
# point into freed memory.
assert('an mruby::root that outlives its state reads nil') do
  assert_true MrubyCppTest.root_after_close_is_nil?
end

# mruby aborts the process for a raise with no handler above it
# (src/error.c, exc_throw). mruby::protect gives the raise back as a value.
assert('mruby::protect gives a raise back as a value') do
  assert_true MrubyCppTest.protect_returns_raise?
end

# dup and clone copy the instance variables of a data object, and not the
# C++ object. Nobody can know whether a copy of the C++ object is safe.
assert('a mruby::data object refuses dup and clone') do
  counted = MrubyCppTest.counted_new
  assert_raise(NotImplementedError) { counted.dup }
  assert_raise(NotImplementedError) { counted.clone }
  assert_true MrubyCppTest.wrap_in_other_class_throws?
end

# A value that a C++ object keeps lives on its Ruby holder, under the name
# of the C++ field, where Ruby cannot reach it. Each operation mirrors what
# C++ does with the field.
assert('mruby::kept mirrors the field of a C++ object') do
  holder = Object.new
  first = 'a'
  second = 'b'
  assert_equal first, MrubyCppTest.kept_after(holder, :assign, first)
  assert_equal [], holder.instance_variables
  holder = Object.new
  assert_equal [first], MrubyCppTest.kept_after(holder, :push_back, first)
  assert_equal [first, second], MrubyCppTest.kept_after(holder, :push_back, second)
  assert_equal [second], MrubyCppTest.kept_after(holder, :erase, first)
  assert_nil MrubyCppTest.kept_after(holder, :clear, nil)
end

# mruby says nothing when it frees an object, but it calls dfree for a data
# object. mruby::data holds the C++ object in a std::shared_ptr, so a
# std::weak_ptr to it expires when the collector frees the Ruby object.
assert('a std::weak_ptr to a mruby::data expires with the Ruby object') do
  assert_true MrubyCppTest.data_weak_expires?
end

# mrb_intern_static and mrb_str_new_static keep the pointer they are given,
# so the bytes must live as long as the state. A literal does.
assert('mruby::symbol and mruby::str_new_static read a literal') do
  assert_true MrubyCppTest.literals?
end
