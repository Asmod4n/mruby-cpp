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

# Ruby's freeze also freezes the singleton class, so that no singleton
# method can be added. mruby::frozen does the same, and thaws both after.
assert('mruby::frozen freezes the singleton class as freeze does') do
  object = Object.new
  def object.x; end
  assert_equal [[true, true], [false, false]], MrubyCppTest.singleton_frozen(object)
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
  # The ivar is bookkeeping of the C++ field and not a change the user
  # made, so it works on a holder the user froze, which stays frozen.
  frozen = Object.new.freeze
  assert_equal [first], MrubyCppTest.kept_after(frozen, :push_back, first)
  assert_true frozen.frozen?
  assert_nil MrubyCppTest.kept_after(Object.new, :erase, first)
  assert_true MrubyCppTest.kept_on_integer_throws?
end

# mrb_intern_static and mrb_str_new_static keep the pointer they are given,
# so the bytes must live as long as the state. A literal does.
assert('mruby::symbol and mruby::str_new_static read a literal') do
  assert_true MrubyCppTest.literals?
end
