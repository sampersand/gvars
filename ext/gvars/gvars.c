#include "ruby.h"
#define dbg(...) (fprintf(stderr, "%s:%s:%d: ", __FILE__, __FUNCTION__, __LINE__), fprintf(stderr,__VA_ARGS__),fprintf(stderr,"\n"),fflush(stderr))

VALUE gvars_module;

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

struct value_ptr { VALUE *value; };
static void value_ptr_mark(void *ptr) { rb_gc_mark(*((struct value_ptr *) ptr)->value); }
static void value_ptr_free(void *ptr) { ruby_xfree(((struct value_ptr *) ptr)->value); }
static const rb_data_type_t value_ptr_data_type = {
	.wrap_struct_name = "gvars/value_ptr",
	.function = {
		.dmark = value_ptr_mark,
		.dfree = value_ptr_free,
		.dsize = NULL // todo: not 0
	},
	.flags = RUBY_TYPED_FREE_IMMEDIATELY
};


struct gvars_virtual_var {
	int pass_args;
	VALUE backing, getter, setter;
};

static void gvars_virtual_var_mark(void *ptr) {
	struct gvars_virtual_var *gv = ptr;
	if (gv->backing != Qundef) rb_gc_mark(gv->backing);
	rb_gc_mark(gv->getter);
	rb_gc_mark(gv->setter);
}

static void gvars_virtual_var_free(void *ptr) {
	xfree(ptr);
}

static const rb_data_type_t gvars_type = {
	.wrap_struct_name = "gvars_var",
	.function = {
		.dmark = gvars_virtual_var_mark,
		.dfree = gvars_virtual_var_free,
		.dsize = NULL
	},
	.flags = RUBY_TYPED_FREE_IMMEDIATELY
};


static VALUE virtual_var_getter(ID id, VALUE *data) {
	struct gvars_virtual_var *gv;
    TypedData_Get_Struct(*data, struct gvars_virtual_var, &gvars_type, gv);

    if (gv->pass_args) {
		return rb_proc_call_kw(gv->getter, rb_ary_new3(1, rb_id2str(id)), RB_NO_KEYWORDS);
    } else {
		return rb_proc_call_kw(gv->getter, rb_ary_new(), RB_NO_KEYWORDS);
	}
}

static void virtual_var_setter(VALUE val, ID id, VALUE *data) {
	struct gvars_virtual_var *gv;
    TypedData_Get_Struct(*data, struct gvars_virtual_var, &gvars_type, gv);

	rb_proc_call_kw(gv->setter, rb_ary_new3(2, rb_id2str(id), val), RB_NO_KEYWORDS);
}

static VALUE gvars_virtual_var_alloc(VALUE backing, VALUE getter, VALUE setter) {
	struct gvars_virtual_var *gv;
	VALUE gvar_ty = TypedData_Make_Struct(rb_cObject, struct gvars_virtual_var, &gvars_type, gv);
	gv->backing = backing;
	gv->getter = getter;
	gv->setter = setter;
	gv->pass_args = 0;
	dbg("%p", gv);
	return gvar_ty;
}

static void
gvars_define_virtual_method(VALUE self, VALUE *name, VALUE backing, VALUE getter, VALUE setter)
{
	char *name_str = get_global_name(name);
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

	struct value_ptr *vp;
	VALUE vp_data = TypedData_Make_Struct(rb_cObject, struct value_ptr, &value_ptr_data_type, vp);
	vp->value = RB_ALLOC(VALUE);
	*vp->value = gvars_virtual_var_alloc(backing, getter, setter);

	rb_define_hooked_variable(name_str, vp->value, virtual_var_getter, setter_proc == Qnil ? rb_gvar_readonly_setter : virtual_var_setter);

  	VALUE hash = rb_iv_get(gvars_module, "@vars");
  	VALUE namesym = rb_str_new_cstr(name_str);
  	rb_hash_aset(hash, namesym, vp_data);
}

static VALUE
gvars_f_define_virtual_method(int argc, VALUE *argv, VALUE self)
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

	gvars_define_virtual_method(self, &name, Qundef,getter, setter);

	return name; //TODO: ID2SYM(id)
}

void
Init_gvars(void)
{
	gvars_module = rb_define_module("GVars");
  	rb_ivar_set(gvars_module, rb_intern("@vars"), rb_hash_new());

	// Define module-level functions that can be used as mixins
	rb_define_module_function(gvars_module, "global_variable_get", gvars_f_global_variable_get, 1);
	rb_define_module_function(gvars_module, "global_variable_set", gvars_f_global_variable_set, 2);
	rb_define_module_function(gvars_module, "alias_global_variable", gvars_f_alias_global_variable, 2);

	// Don't make it an instance method, as it exists in Kernel
	rb_define_singleton_method(gvars_module, "global_variables", gvars_f_global_variables, 0);

	// Don't make it an instance method, cause we don't want it included
	rb_define_singleton_method(gvars_module, "define_virtual_method", gvars_f_define_virtual_method, -1);

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
	rb_define_alias(gvars_singleton, "virtual", "define_virtual_method");
}
