# C++ that reads the bytes of a Ruby object needs them to stay where they
# are for as long as it reads. mruby checks the frozen flag before it
# changes or moves the buffer of a String, an Array or a Hash, so a lock
# that freezes the object for a scope holds the buffer in place.
assert('mruby::value freezes an object while a lock holds it') do
  text = 'abc'
  assert_true MrubyCppTest.frozen_while_locked?(text)
  assert_false text.frozen?
  assert_true MrubyCppTest.frozen_while_locked?([1, 2])
end

# A change while the lock holds the object raises, and the end of the
# scope thaws the object also when the scope ends with that raise.
assert('mruby::value refuses a change while locked, and thaws on a raise') do
  text = 'abc'
  assert_raise(FrozenError) { MrubyCppTest.append_while_locked(text) }
  assert_false text.frozen?
  assert_equal 'abc', text
end

# An object that was frozen before the lock stays frozen after it.
assert('mruby::value leaves a frozen object frozen') do
  frozen = 'abc'.freeze
  assert_true MrubyCppTest.frozen_while_locked?(frozen)
  assert_true frozen.frozen?
end

# One mruby::value can be locked twice. The object stays frozen until the
# outer lock ends.
assert('mruby::value stays frozen until the last lock ends') do
  text = 'abc'
  assert_true MrubyCppTest.frozen_after_inner_lock?(text)
  assert_false text.frozen?
end

# An mrb_state belongs to one thread. The frozen flag is a plain field, so
# a lock from another thread would race with that thread.
assert('mruby::value refuses a lock from another thread') do
  assert_true MrubyCppTest.lock_from_other_thread_throws?('abc')
end

# An Integer, a Symbol and nil have no buffer, and a lock does not touch
# them.
assert('mruby::value leaves immediate values alone') do
  assert_false MrubyCppTest.frozen_while_locked?(1)
  assert_false MrubyCppTest.frozen_while_locked?(nil)
end
