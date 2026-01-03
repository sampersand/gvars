# frozen_string_literal: true

require "test_helper"

class TestGVars < Minitest::Test
  def test_that_it_has_a_version_number
    refute_nil ::GVars::VERSION
  end

  # Helper to yield different forms of gvar names that are accepted
  def with_interned(name)
    yield name.to_sym
    yield name.to_s
    yield Blankity::To.str(name.to_s)
  end

  # Helper to get a match data without polluting the current scope's
  def get_a_match_data
    'hello'.match(/h(e)(ll)/)
  end

  # Helper to assert that a method is an alias of another
  def assert_alias(cls, old, new)
    assert_equal old, cls.singleton_method(new).original_name
  end

  def test_get_normal
    $test_global_variable_get = :foo

    assert_alias GVars, :global_variable_get, :get
    assert_alias GVars, :global_variable_get, :[]

    with_interned :$test_global_variable_get do |gvar|
      assert_equal :foo, GVars.get(gvar)
    end

    # Ignore warnings about undefined gvars
    capture_io do
      with_interned :$test_global_variable_unset do |gvar|
        assert_nil GVars.get(gvar)
      end
    end

    # Require a `$` beforehand
    with_interned :not_a_gvar do |gvar|
      assert_raises NameError do
        GVars.get(gvar)
      end
    end

    # Make sure it only accepts `interned`s
    assert_raises TypeError do
      GVars.get(Blankity::To.s('$foo'))
    end
  end

  # Make sure the special scope-local globals (regex ones and `$_`) work
  def test_get_scope_local
    assert_nil GVars.get(:$_)
    assert_nil GVars.get(:$+)

    $_ = 'hello'
    assert_equal 'hello', GVars.get(:$_)

    ~/(he)(..)o/
    assert_equal 'll', GVars.get(:$+)
  end

  # Make sure we can get virtual variables
  def test_get_builtin_virtual
    # Use exception variables as a test proxy
    assert_nil GVars.get(:$@)
    assert_nil GVars.get(:$!)

    begin
      raise "test"
    rescue
      refute_nil GVars.get(:$@)
      refute_nil GVars.get(:$!)
    else
      flunk
    end
  end

  # Make sure we can use utf8, and not just ascii, in our variable names
  def test_get_utf8
    skip 'TODO: get utf8 working' # unfortunately, `rb_gv_get` doesn't support it.

    $test_get_utf8_π = 'pi!'
    assert_equal 'pi!', GVars.get(:$test_get_utf8_π)
  end

  def test_set_normal
    assert_alias GVars, :global_variable_set, :set
    assert_alias GVars, :global_variable_set, :[]=

    # Also tests unset ones, as the initial round will be unset
    with_interned :$test_global_variable_set do |gvar|
      # Make sure it's set to the value it's supplied
      assert_equal :bar, GVars.set(gvar, :bar)
    end

    # Require a `$` beforehand
    with_interned :not_a_gvar do |gvar|
      assert_raises NameError do
        GVars.set(gvar, :bar)
      end
    end

    # Make sure it only accepts `interned`s
    assert_raises TypeError do
      GVars.set(Blankity::To.s('$foo'), :bar)
    end
  end

  def test_set_scope_local
    assert_nil $_

    assert_equal "hi", GVars.set(:$_, "hi")
    assert_equal "hi", $_

    assert_nil $~
    md = get_a_match_data
    assert_equal md, GVars.set(:$~, md)
    assert_equal md, $~
    assert_equal 'll', $+
  end

  # Make sure we can set virtual variables
  def test_set_builtin_virtual
    # Use backtrace as the value we're assigning
    assert_nil $@

    begin
      raise "test"
    rescue
      GVars.set(:$@, ['lol'])
      assert_equal ['lol'], $@
    else
      flunk
    end
  end

  # Make sure we can use utf8, and not just ascii, in our variable names
  def test_set_utf8
    skip 'TODO: set utf8 working' # unfortunately, `rb_gv_set` doesn't support it.

    assert_equal 'pi!', GVars.set(:$test_get_utf8_π, 'pi!')
    assert_equal 'pi!', $test_get_utf8_π
  end

  def test_list
    assert_alias GVars, :global_variables, :list

    orig = GVars.list
    $new_value_with_utf8_π = 'hi pi!'
    assert_equal [:$new_value_with_utf8_π], GVars.list - orig
  end

  def test_alias_normal
    assert_alias GVars, :alias_global_variable, :alias

    # They're unset, so ignore the warning
    capture_io do
      assert_nil $test_alias_normal1
      assert_nil $test_alias_normal2
    end

    $test_alias_normal2 = :one
    with_interned :$test_alias_normal1 do |gvar1|
      with_interned :$test_alias_normal2 do |gvar2|
        assert_equal :$test_alias_normal1, GVars.alias(gvar1, gvar2)
        assert_equal GVars.get(gvar1), GVars.get(gvar2)
        assert_equal :one, GVars.get(gvar1)
      end
    end

    # Require a `$` beforehand
    with_interned :not_a_gvar do |gvar|
      assert_raises NameError do GVars.alias(gvar, :$bar) end
      assert_raises NameError do GVars.alias(:$bar, gvar) end
    end

    # Make sure it only accepts `interned`s
    assert_raises TypeError do
      GVars.alias(Blankity::To.s('$foo'), :$bar)
    end
    assert_raises TypeError do
      GVars.alias(:$bar, Blankity::To.s('$foo'))
    end
  end

  def test_each
    GVars.each do |key, value|
      if value.nil?
        assert_nil GVars.get(key)
      else
        assert_equal value, GVars.get(key)
      end
    end
  end

  def test_includes_enumerable
    # Make sure we can do `find`
    $test_includes_enumerable = o = Object.new
    assert_equal [:$test_includes_enumerable, o], GVars.find { _2 == o }
  end
end
