# frozen_string_literal: true

require "test_helper"

class TestGVars < Minitest::Test
  def test_that_it_has_a_version_number
    refute_nil ::GVars::VERSION
  end

  def with_gvar(name)
    yield name.to_sym
    yield name.to_s
    yield Blankity::To.str(name.to_s)
  end

  def assert_alias(cls, old, new)
    assert_equal old, cls.singleton_method(new).original_name
  end

  def test_get
    $test_global_variable_get = :foo

    assert_alias GVars, :global_variable_get, :get
    assert_alias GVars, :global_variable_get, :[]

    with_gvar :$test_global_variable_get do |gvar|
      assert_equal :foo, GVars.get(gvar)
    end

    # Ignore warnings about undefined gvars
    capture_io do
      with_gvar :$test_global_variable_unset do |gvar|
        assert_nil GVars.get(gvar)
      end
    end

    # Require a `$` beforehand
    with_gvar :not_a_gvar do |gvar|
      assert_raises NameError do
        GVars.get(gvar)
      end
    end

    # Make sure it only accepts `interned`s
    assert_raises TypeError do
      GVars.get(12)
    end
  end

  def test_set
    assert_alias GVars, :global_variable_set, :set
    assert_alias GVars, :global_variable_set, :[]=

    # Also tests unset ones, as the initial round will be unset
    with_gvar :$test_global_variable_set do |gvar|
      assert_equal :bar, GVars.set(gvar, :bar)
    end

    # Require a `$` beforehand
    with_gvar :not_a_gvar do |gvar|
      assert_raises NameError do
        GVars.set(gvar, :bar)
      end
    end

    # Make sure it only accepts `interned`s
    assert_raises TypeError do
      GVars.set(12, :bar)
    end
  end
end

__END__
void
Init_gvars(void)
{
  gvars_module = rb_define_module("GVars");

  // Define module-level functions that can be used as mixins
  rb_define_module_function(gvars_module, "global_variable_get", gvars_f_global_variable_get, 1);
  rb_define_module_function(gvars_module, "global_variable_set", gvars_f_global_variable_set, 2);
  rb_define_module_function(gvars_module, "alias_global_variable", gvars_f_alias_global_variable, 2);

  // Don't make mixin, as it exists in Kernel
  rb_define_singleton_method(gvars_module, "global_variables", gvars_f_global_variables, 0);

  // Singleton aliases (i.e. not used in the mixin)
  rb_define_alias(rb_singleton_class(gvars_module), "get", "global_variable_get");
  rb_define_alias(rb_singleton_class(gvars_module), "[]", "global_variable_get");
  rb_define_alias(rb_singleton_class(gvars_module), "set", "global_variable_set");
  rb_define_alias(rb_singleton_class(gvars_module), "[]=", "global_variable_set");
  rb_define_alias(rb_singleton_class(gvars_module), "alias", "alias_global_variable");
  rb_define_alias(rb_singleton_class(gvars_module), "list", "global_variables");
}
