require 'gvars'
p GVars::VERSION

100.times do
  GVars.virtual(:$foo) { p "bar" }
  # GVars.virtual(:$foo) { p "foo" }
  # $foo
  p($foo)
  p GVars.instance_variable_get(:@vars)
  GC.start
end
# GVars.virtual(:$bar, proc{ p "bar" })
# GVars.virtual(:$baz, proc{ p "bar" }, proc { p "baz: #{_1}"})

# 100.times do
# $foo
# $bar
# $baz
# GC.start
# end
