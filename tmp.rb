require 'gvars'
p GVars::VERSION

GVars.virtual(:$foo) { p "bar" }
# GVars.virtual(:$foo) { p "foo" }
# $foo
print($foo)
# GVars.virtual(:$bar, proc{ p "bar" })
# GVars.virtual(:$baz, proc{ p "bar" }, proc { p "baz: #{_1}"})

# 100.times do
# $foo
# $bar
# $baz
# GC.start
# end
