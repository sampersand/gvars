# GVars

Adds in missing utilities relating to global variables in Ruby.

Ever been bothered how there's `instance_variable_get`, `binding.local_variable_get`, `class_variable_get`, even `const_get`, but no `global_variable_get`? Ever resorted to `eval("$#{name} = value")` to assign a global variable? Look no further!

## Features
```ruby
$foo = 34

# Dynamically lookup globals
puts GVars.get(:$foo) #=> 34

# Dynamically assign globals!
GVars.set(:$foo, 99)
puts $foo #=> 99

# How about dynamically `alias`ing globals?
GBars.alias :$bar, :$foo
$bar = 12
puts $foo #=> 12

# You can also mixin `GVars` to get the methods that should be
# defined on Kernel:
include GVars
puts global_variable_get(:$bar) #=> 12
puts global_variable_set(:$bar, 19) #=> 19
```

Up next: virtual variables, and collection methods!

## Known problems
Unfortunately, Ruby's C-level `rb_gv_get` / `rb_gv_set` methods only let you manipulate ASCII identifiers... A fix _may_ be possible, but it'll have to resort to the `eval` hack.
