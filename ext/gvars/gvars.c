#include "ruby.h"

static VALUE
gvars_global_variable_get(VALUE self, VALUE name)
{
	if (RB_SYMBOL_P(name)) name = rb_sym2str(name);
	return rb_gv_get(StringValueCStr(name));
}

static VALUE
gvars_global_variable_set(VALUE self, VALUE name, VALUE value)
{
	if (RB_SYMBOL_P(name)) name = rb_sym2str(name);
	return rb_gv_set(StringValueCStr(name), value);
}

static VALUE
gvars_global_variable_defined_p(VALUE self, VALUE name)
{
	extern VALUE rb_gvar_defined(ID);
	return rb_gvar_defined(rb_check_id(&name));
}

static VALUE
gvars_global_variables(VALUE self)
{
	return rb_f_global_variables();
}

static VALUE
gvars_alias_global_variable(VALUE self, VALUE new, VALUE old)
{
	if (RB_SYMBOL_P(new)) new = rb_sym2str(new);
	if (RB_SYMBOL_P(old)) old = rb_sym2str(old);

	// ID newid = rb_intern_str(new);
	// ID newid = rb_check_id(&new);
	// ID oldid = rb_check_id(&old);

	rb_alias_variable(rb_intern_str(new), rb_intern_str(old));
	return new;
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
	VALUE name, getter, setter;

	switch (rb_scan_args(argc, argv, "12", &name, &getter, &setter)) {
	case 1: getter = (rb_need_block(), rb_block_proc());
	case 2: setter = Qnil;
	case 3: break;
	default: rb_bug("oops");
	}

	const char *name_str = rb_id2name(rb_check_id(&name));

	VALUE getter_proc, setter_proc;

	getter_proc = rb_convert_type(getter, T_DATA, "Proc", "to_proc");
	if (NIL_P(getter_proc) || !rb_obj_is_proc(getter_proc)) {
		rb_raise(rb_eTypeError, "wrong getter type %s (expected Proc)", rb_obj_classname(getter_proc));
	}

	if (NIL_P(setter)) {
		setter_proc = Qnil;
	} else {
		setter_proc = rb_convert_type(setter, T_DATA, "Proc", "to_proc");
		if (NIL_P(setter_proc) || !rb_obj_is_proc(setter_proc)) {
			rb_raise(rb_eTypeError, "wrong setter type %s (expected Proc)", rb_obj_classname(setter_proc));
		}
	}

	// todo: not just leak memory here lol
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
