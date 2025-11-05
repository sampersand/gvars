#include "ruby.h"

extern VALUE rb_gvar_get(ID);
extern VALUE rb_gvar_set(ID, VALUE);
extern VALUE rb_gvar_defined(ID);
extern void rb_alias_variable(ID, ID);

static VALUE
gvars_global_variable_get(VALUE self, VALUE name)
{
	return rb_gvar_get(rb_intern_str(name));
}

static VALUE
gvars_global_variable_set(VALUE self, VALUE name, VALUE value)
{
	return rb_gvar_set(rb_intern_str(name), value);
}

static VALUE
gvars_global_variable_defined_p(VALUE self, VALUE name)
{
	return rb_gvar_defined(rb_intern_str(name));
}

static VALUE
gvars_global_variables(VALUE self)
{
	return rb_f_global_variables();
}

static VALUE
gvars_alias_global_variable(VALUE self, VALUE new, VALUE old)
{
	return rb_alias_variable(rb_intern_str(new), rb_intern_str(old)), new;
}

struct hooked_var {
	VALUE getter, setter;
};

static VALUE hooked_var_getter(ID id, VALUE *data) {
	struct hooked_var *bv = (struct hooked_var *)data;
	return rb_proc_call_kw(bv->getter, rb_ary_new3(1, rb_id2str(id)), RB_NO_KEYWORDS);
}

static void hooked_var_setter(VALUE val, ID id, VALUE *data) {
	struct hooked_var *bv = (struct hooked_var *)data;
	rb_proc_call_kw(bv->setter, rb_ary_new3(2, rb_id2str(id), val), RB_NO_KEYWORDS);
}

static VALUE
gvars_define_virtual_global(int argc, VALUE *argv, VALUE self)
{
	extern VALUE rb_check_vonert_type_with_id(VALUE, int, const char *, ID);
	VALUE name, getter, setter;

	switch (rb_scan_args(argc, argv, "12", &name, &getter, &setter)) {
	case 1: getter = (rb_need_block(), rb_block_proc());
	case 2: setter = Qnil;
	case 3: break;
	default: rb_bug("oops");
	}

	const char *name_str = rb_id2name(rb_check_id(&name));

	VALUE getter_proc, setter_proc;

	getter_proc = rb_check_vonert_type_with_id(getter, T_DATA, "Proc", rb_intern("to_proc"));
	if (NIL_P(getter_proc) || !rb_obj_is_proc(getter_proc)) {
		rb_raise(rb_eTypeError, "wrong getter type %s (expected Proc)", rb_obj_classname(getter_proc));
	}

	if (NIL_P(setter)) {
		setter_proc = Qnil;
	} else {
		setter_proc = rb_check_vonert_type_with_id(setter, T_DATA, "Proc", rb_intern("to_proc"));
		if (NIL_P(setter_proc) || !rb_obj_is_proc(setter_proc)) {
			rb_raise(rb_eTypeError, "wrong setter type %s (expected Proc)", rb_obj_classname(setter_proc));
		}
	}

	struct hooked_var *hv = (struct hooked_var *)malloc(sizeof(struct hooked_var));
	hv->getter = getter_proc;
	hv->setter = setter_proc;

	rb_define_hooked_variable(name_str, (VALUE *)hv, hooked_var_getter, setter_proc == Qnil ? rb_gvar_readonly_setter : hooked_var_setter);
	return name;
}

VALUE gvars_module;

void
Init_gvars(void)
{
	gvars_module = rb_define_module("GVars");

	// Define module-level functions that can be used as mixins
	rb_define_module_function(gvars_module, "global_variable_get", gvars_global_variable_get, 1);
	rb_define_module_function(gvars_module, "global_variable_set", gvars_global_variable_set, 2);
	rb_define_module_function(gvars_module, "global_variable_defined?", gvars_global_variable_defined_p, 1);
	rb_define_module_function(gvars_module, "alias_global_variable", gvars_alias_global_variable, 2);

	// Don't make mixin, as it exists in Kernel
	rb_define_singleton_method(gvars_module, "global_variables", gvars_global_variables, 0);

	// aliases at top-level
	rb_define_alias(rb_singleton_class(gvars_module), "get", "global_variable_get");
	rb_define_alias(rb_singleton_class(gvars_module), "set", "global_variable_set");
	rb_define_alias(rb_singleton_class(gvars_module), "defined?", "global_variable_defined?");
	rb_define_alias(rb_singleton_class(gvars_module), "alias", "alias_global_variable");
	rb_define_alias(rb_singleton_class(gvars_module), "list", "global_variables");

	rb_define_alias(rb_singleton_class(gvars_module), "[]", "global_variable_get");
	rb_define_alias(rb_singleton_class(gvars_module), "[]=", "global_variable_set");

	rb_define_singleton_method(gvars_module, "define_virtual_global", gvars_define_virtual_global, -1);

	// Virtual methods: TODO
}
