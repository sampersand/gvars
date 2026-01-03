#include "ruby.h"

// Converts `name` to a global variable name, ensures it's valid, and returns it.
//
// for checking, it makes that `name` starts with `$`. This isn't really required, as ruby supports
// globals that don't start with `$` (but doesn't expose any methods to interact with them)
static char *get_global_name(VALUE *name) {
	// If `name` is a symbol, get its backing string
	if (RB_SYMBOL_P(*name)) {
		*name = rb_sym2str(*name);
	}

	char *namestr = StringValueCStr(*name);

	// Ensure it starts with a dollar sign
	if (namestr[0] != '$') {
		rb_raise(rb_eNameError, "'%s' is not allowed as a global variable name", namestr);
	}

	return namestr;
}

static VALUE
gvars_f_global_variable_get(VALUE self, VALUE name)
{
	return rb_gv_get(get_global_name(&name));
}

static VALUE
gvars_f_global_variable_set(VALUE self, VALUE name, VALUE value)
{
	return rb_gv_set(get_global_name(&name), value);
}

static VALUE
gvars_f_global_variables(VALUE self)
{
	return rb_f_global_variables();
}

static VALUE
gvars_f_alias_global_variable(VALUE self, VALUE new, VALUE old)
{
    ID newid = rb_intern(get_global_name(&new));
	rb_alias_variable(newid, rb_intern(get_global_name(&old)));
	return ID2SYM(newid);
}

VALUE gvars_module;

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
}
