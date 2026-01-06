require 'gvars'
p GVars::VERSION

100.times do
  GVars.virtual(:$foo) { p "bar" }
  # GVars.virtual(:$foo) { p "foo" }
  # $foo
  # p($foo)
  # p GVars.instance_variable_get(:@vars)
  GC.start
end

GVars.virtual(:$foo1) { "hello" }; fail unless 'hello' == $foo1
GVars.virtual(:$foo2, proc{ "hello" }); fail unless 'hello' == $foo2
GVars.virtual(:$foo3, proc{ "hello" }, proc { it + 1 }); fail unless ['hello', 3, 'hello'] == [$foo3, $foo3 = 3, $foo3]

GVars.virtual(:$bar1, initial: 0) { it + 1 }; fail unless [1, 2, 3] == [$bar1, $bar1, $bar1]
$bar1 = 10
fail unless 11 == $bar1

GVars.virtual(:$bar2, state: 'a') { it.succ!.dup }
fail unless ['b', 'c', 'd'] == [$bar2, $bar2, $bar2]
$bar2 = 10
fail unless 11 == $bar2

# GVars.virtual(:$bar, proc{ p "bar" })
# GVars.virtual(:$baz, proc{ p "bar" }, proc { p "baz: #{_1}"})

# 100.times do
# $foo
# $bar
# $baz
# GC.start
# end
