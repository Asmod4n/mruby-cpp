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

# A known limit: mruby::frozen thaws what it froze when it ends, also when
# Ruby code called freeze on the object inside the scope. mruby tells
# nobody about a freeze, so mruby::frozen cannot see one.
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

# mrb_gc_unregister only looks the object up in a hash table and counts
# down (src/gc.c), so the deleter of a root calls it at once, and the
# collector then frees the object.
assert('a released mruby::root is collected') do
  assert_true MrubyCppTest.released_root_is_collected?
end

# mrb_close frees every object. A root that outlives its state must not
# point into freed memory. It reads undef and not nil, because nil is a
# value a root can legally hold.
assert('an mruby::root that outlives its state reads undef') do
  assert_true MrubyCppTest.root_after_close_is_undef?
end

# mrb_close frees the table of roots before it frees the heap (src/gc.c,
# mrb_gc_destroy), and the dfree of a data object can release a root on
# its way out. mrb_gc_unregister returns at once when the table is gone.
assert('a root released by a dfree inside mrb_close') do
  assert_true MrubyCppTest.root_released_in_close?
end

# mruby aborts the process for a raise with no handler above it
# (src/error.c, exc_throw). mruby::protect gives the raise back as a value.
assert('mruby::protect gives a raise back as a value') do
  assert_true MrubyCppTest.protect_returns_raise?
end

# mrb_protect_error sets mrb->jmp for the time of the call. A C++ exception
# that passes through it leaves mrb->jmp pointing at a frame that is gone,
# and the next raise jumps there. mruby::protect carries the exception past
# mrb_protect_error and throws it again after, because only the C++ code
# that threw it can repair the cause.
assert('mruby::protect gives a C++ exception back as itself') do
  assert_true MrubyCppTest.protect_rethrows_cxx_exception?
end

# mrb_intern_static and mrb_str_new_static keep the pointer they are given,
# so the bytes must live as long as the state. A literal does.
assert('mruby::symbol and mruby::str_new_static read a literal') do
  assert_true MrubyCppTest.literals?
end
