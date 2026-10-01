# A value that C++ keeps beyond a call is held by a root. Only the root
# holds it here, so the collector must not free it.
assert('mruby::root keeps a value through a full collection') do
  assert_true MrubyCppTest.root_keeps_value?
end

# A destructor may not run mruby, so releasing a root only lists it. The
# next root unregisters it, and the collector then frees the object.
assert('a released mruby::root is collected after the next root') do
  assert_true MrubyCppTest.released_root_is_collected?
end

# mrb_close frees every object. A root that outlives its state must not
# point into freed memory, so the state turns it into nil when it closes.
assert('an mruby::root that outlives its state reads nil') do
  assert_true MrubyCppTest.root_after_close_is_nil?
end

# A value that mruby made in the arena lives until the mruby call returns.
# A copy, a move or new would carry it past that point.
assert('mruby::automatic cannot leave the call that made it') do
  assert_true MrubyCppTest.automatic_cannot_escape?
end
