#include "ruby.h"

// Converts `name` to a global variable name, ensures it's valid, and returns it.
//
// for checking, it makes that `name` starts with `$`. This isn't really required, as ruby supports
// globals that don't start with `$` (but doesn't expose any methods to interact with them)
static char *get_global_name(VALUE *name) {
	if (RB_SYMBOL_P(*name)) *name = rb_sym2str(*name);
	char *namestr = StringValueCStr(*name);

	if (namestr[0] != '$') {
		rb_raise(rb_eNameError, "'%s' is not allowed as a global variable name", namestr);
	}

	return namestr;
}

static VALUE
gvars_f_get(VALUE self, VALUE name)
{
	return rb_gv_get(get_global_name(&name));
}

static VALUE
gvars_f_set(VALUE self, VALUE name, VALUE value)
{
	return rb_gv_set(get_global_name(&name), value);
}

static VALUE
gvars_f_defined_p(VALUE self, VALUE name)
{
	extern VALUE rb_gvar_defined(ID);
	return rb_gvar_defined(rb_check_id(&name));
}

static VALUE
gvars_list(VALUE self)
{
	return rb_f_global_variables();
}

static VALUE
gvars_f_alias(VALUE self, VALUE new, VALUE old)
{
    ID newid = rb_intern(get_global_name(&new));
	rb_alias_variable(newid, rb_intern(get_global_name(&old)));
	return ID2SYM(newid);
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
	case 1:
		getter = (rb_need_block(), rb_block_proc());
		setter = Qnil;
		break;

	case 2:
		setter = Qnil;

	case 3:
        if (rb_block_given_p()) {
            rb_warn("given block not used");
        }
		break;
	default:
		rb_bug("oops");
	}

	char *name_str = get_global_name(&name);

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
extern void rb_gvar_ractor_local(const char *);
	rb_gvar_ractor_local(name_str);
	return name; //TODO: ID2SYM(id)
}

VALUE gvars_module;

void
Init_gvars(void)
{
	gvars_module = rb_define_module("GVars");

	// Define module-level functions that can be used as mixins
	rb_define_module_function(gvars_module, "global_variable_get", gvars_f_get, 1);
	rb_define_module_function(gvars_module, "global_variable_set", gvars_f_set, 2);
	rb_define_module_function(gvars_module, "global_variable_defined?", gvars_f_defined_p, 1);
	rb_define_module_function(gvars_module, "alias_global_variable", gvars_f_alias, 2);

	// Don't make mixin, as it exists in Kernel
	rb_define_singleton_method(gvars_module, "global_variables", gvars_list, 0);

	// aliases at top-level
	rb_define_alias(rb_singleton_class(gvars_module), "get", "global_variable_get");
	rb_define_alias(rb_singleton_class(gvars_module), "set", "global_variable_set");
	rb_define_alias(rb_singleton_class(gvars_module), "defined?", "global_variable_defined?");
	rb_define_alias(rb_singleton_class(gvars_module), "alias", "alias_global_variable");
	rb_define_alias(rb_singleton_class(gvars_module), "list", "global_variables");
	rb_define_alias(rb_singleton_class(gvars_module), "[]", "global_variable_get");
	rb_define_alias(rb_singleton_class(gvars_module), "[]=", "global_variable_set");

	rb_define_singleton_method(gvars_module, "virtual", gvars_define_virtual_global, -1);

	// Virtual methods: TODO
}
