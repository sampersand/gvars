# GVars

Adds in missing utilities relating to global variables in Ruby.

Ever been bothered how there's `instance_variable_get`, `binding.local_variable_get`, `class_variable_get`, even `const_get`, but no `global_variable_get`? Ever resorted to `eval("$#{name} = value")` to assign a global variable? Look no further!

## Features

`GVars` adds support for getting, setting, and aliasing global variables dynamically. (Checking for definition is coming!)
```ruby
# Dynamically interact with globals:
GVars.set(:$foo, 99)
puts GVars.get(:$foo) #=> 99

GVars.alias(:$bar, :$foo)
$bar = 12
puts $foo #=> 12

# Mixin `GVars` to get methods that should've been defined
# on `Kernel` all along
Kernel.include GVars
puts global_variable_get(:$bar) #=> 12
puts global_variable_set(:$bar, 19) #=> 19
```

It also supports virtual variables, which evaluate a `proc` each time:
```ruby
# Get the current time whenever you access `$TIME`
GVars.virtual(:$TIME) { Time.now }
puts $TIME.monday? #=> true

# You can also provide a custom setter
GVars.virtual(:$PWD,
	getter: ->{ Dir.pwd },
	setter: ->new{ Dir.chdir(new) })
$PWD = '/tmp'
p $PWD #=> "/private/tmp" (macOS symlinks `/tmp` to `/private/tmp`)
```

And, hooked variables:
```ruby
# Initial values compute their "backing" value based on the getter's
# return value.
GVars.hooked(:$COUNTER, initial: 0) { it + 1 }
p [$COUNTER, $COUNTER, $COUNTER] #=> [1, 2, 3]

# If you don't supply a setter, it defaults to just the backing value.
$COUNTER = 10
p $COUNTER #=> 11

# You can also specify a state, which is given to the getter (and setter),
# but isn't updated by the getter's return value.
GVars.hooked(:$RAND,
	state: Random.new,
	getter: ->random{ random.rand },
	setter: ->(value,random){ random.srand(value) }) # doesn't actually work.. `Random#srand` isn't a thing
```

## Known problems
1. Unfortunately, Ruby's C-level `rb_gv_get` / `rb_gv_set` methods only let you manipulate ASCII identifiers... A fix _may_ be possible, but it'll have to resort to the `eval` hack.
2. Because `$_`, and regex variables (`$~`, ``$` ``, etc) are all local to the function they're called within, you can't actually access them from within procs; As a workaround, you can pass a string that'll be `eval`d each time, but it's not terribly efficient. I don't know if there's a better solution
