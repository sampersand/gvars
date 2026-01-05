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

static VALUE
gvars_enum_length(VALUE self, VALUE args, VALUE eobj)
{
	return LONG2NUM(RARRAY_LEN(rb_f_global_variables()));
}

static VALUE
gvars_f_each(VALUE self)
{
	RETURN_SIZED_ENUMERATOR(self, 0, 0, gvars_enum_length);

	VALUE gvars = gvars_f_global_variables(self);
	for (long i = 0; i < RARRAY_LEN(gvars); i++) {
		VALUE key = RARRAY_AREF(gvars, i);
		rb_yield(rb_ary_new3(2, key, gvars_f_global_variable_get(self, key)));
	}

	return self;
}

static VALUE
gvars_f_to_h(VALUE self)
{
	VALUE gvars = gvars_f_global_variables(self);
	VALUE hash = rb_hash_new_capa(RARRAY_LEN(gvars));

	for (long i = 0; i < RARRAY_LEN(gvars); i++) {
		VALUE key = RARRAY_AREF(gvars, i);
		rb_hash_aset(hash, key, gvars_f_global_variable_get(self, key));
	}

	return hash;
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

	// Don't make it an instance method, as it exists in Kernel
	rb_define_singleton_method(gvars_module, "global_variables", gvars_f_global_variables, 0);

	// Enumerable methods
	rb_extend_object(gvars_module, rb_mEnumerable);
	rb_define_singleton_method(gvars_module, "each", gvars_f_each, 0);
	rb_define_singleton_method(gvars_module, "to_h", gvars_f_to_h, 0);

	// Aliases
	VALUE gvars_singleton = rb_singleton_class(gvars_module);
	rb_define_alias(gvars_singleton, "get", "global_variable_get");
	rb_define_alias(gvars_singleton, "[]", "global_variable_get");
	rb_define_alias(gvars_singleton, "set", "global_variable_set");
	rb_define_alias(gvars_singleton, "[]=", "global_variable_set");
	rb_define_alias(gvars_singleton, "alias", "alias_global_variable");
	rb_define_alias(gvars_singleton, "list", "global_variables");
}
