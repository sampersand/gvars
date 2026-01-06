require 'gvars'
p GVars::VERSION

require 'optparse'
trace_var :$timeout do p it end

GVars.virtual(:$timeout,
  getter: ->{ 'hello' },
  setter: ->val { puts "setting: #{val}" },
  readonly: false)

GVars.hooked(:$COUNTER,
  getter: :succ.to_proc,
  setter: nil)
  setter: ->val { puts "setting: #{val}" },
  initial: 10)

p $timeout1
p $timeout1

__END__

OptParse.new do |op|
  op.on '--timeout=X', 'the timeout'
  op.parse %w[--timeout 34.5], into: GVars
end

p $timeout

__END__
h = {}
GVars.virtual(:$-e, &h.method(:[]).curry(1).<<(:$-e))
# GVars.virtual(:$-e,) { $-W.to_s }
# $-v = false
# p $-e
# $-v = true
# p $-e
# $-v = nil
p $-e

exit

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


GVars.virtual(:$lol) { @x = 3 }
class Thing
  def doit
    $lol
    p @x
  end
end
Thing.new.doit
p @x
      trace_var :$bar, proc {|v| puts "$_ is now '#{v}'" }
      $bar = 2
      $bar = 3
      trace_var :$_, proc {|v| puts "$_ is now '#{v}'" }
    $_ = "hello"
      $_ = ' there'

# $bar2 = 10
# fail unless 11 == $bar2

# GVars.virtual(:$bar, proc{ p "bar" })
# GVars.virtual(:$baz, proc{ p "bar" }, proc { p "baz: #{_1}"})

# 100.times do
# $foo
# $bar
# $baz
# GC.start
# end
