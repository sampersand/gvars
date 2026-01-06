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
		rb_yield(rb_ary_new_from_args(2, key, gvars_f_global_variable_get(self, key)));
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
static size_t value_ptr_size(const void *ptr) { return sizeof(struct value_ptr); }
static const rb_data_type_t value_ptr_data_type = {
	.wrap_struct_name = "gvars/value_ptr",
	.function = {
		.dmark = value_ptr_mark,
		.dfree = value_ptr_free,
		.dsize = value_ptr_size,
	},
	.flags = RUBY_TYPED_FREE_IMMEDIATELY
};


struct gvars_virtual_var {
	VALUE backing, getter, setter;
};

static void gvars_virtual_var_mark(void *ptr) {
	struct gvars_virtual_var *gv = ptr;
	rb_gc_mark(gv->backing);
	rb_gc_mark(gv->getter);
	rb_gc_mark(gv->setter);
}

static size_t gvars_virtual_var_size(const void *ptr) {
	return sizeof(struct gvars_virtual_var);
}

static const rb_data_type_t gvars_type = {
	.wrap_struct_name = "gvars/var",
	.function = {
		.dmark = gvars_virtual_var_mark,
		.dfree = RUBY_DEFAULT_FREE,
		.dsize = gvars_virtual_var_size,
	},
	.flags = RUBY_TYPED_FREE_IMMEDIATELY
};


static VALUE gvars_virtual_var_getter(ID id, VALUE *data) {
	struct gvars_virtual_var *gv;
	TypedData_Get_Struct(*data, struct gvars_virtual_var, &gvars_type, gv);
	return rb_proc_call(gv->getter, rb_ary_new());
}

static void gvars_virtual_var_setter(VALUE val, ID id, VALUE *data) {
	struct gvars_virtual_var *gv;
	TypedData_Get_Struct(*data, struct gvars_virtual_var, &gvars_type, gv);
	rb_proc_call(gv->setter, rb_ary_new_from_args(1, val));
}

static VALUE gvars_hooked_state_var_getter(ID id, VALUE *data) {
	struct gvars_virtual_var *gv;
	TypedData_Get_Struct(*data, struct gvars_virtual_var, &gvars_type, gv);
	return rb_proc_call(gv->getter, rb_ary_new_from_args(1, gv->backing));
}

static VALUE gvars_hooked_initial_var_getter(ID id, VALUE *data) {
	struct gvars_virtual_var *gv;
	TypedData_Get_Struct(*data, struct gvars_virtual_var, &gvars_type, gv);
	return gv->backing = rb_proc_call(gv->getter, rb_ary_new_from_args(1, gv->backing));
}

static void gvars_hooked_var_setter_noproc(VALUE val, ID id, VALUE *data) {
	struct gvars_virtual_var *gv;
	TypedData_Get_Struct(*data, struct gvars_virtual_var, &gvars_type, gv);
	gv->backing = val;
}

static void gvars_hooked_var_setter_proc(VALUE val, ID id, VALUE *data) {
	struct gvars_virtual_var *gv;
	TypedData_Get_Struct(*data, struct gvars_virtual_var, &gvars_type, gv);
	gv->backing = rb_proc_call(gv->setter, rb_ary_new_from_args(2, val, gv->backing));
}

static VALUE gvars_virtual_var_alloc(VALUE backing, VALUE getter, VALUE setter) {
	struct gvars_virtual_var *gv;
	VALUE gvar_ty = TypedData_Make_Struct(rb_cObject, struct gvars_virtual_var, &gvars_type, gv);
	gv->backing = backing;
	gv->getter = getter;
	gv->setter = setter;
	return gvar_ty;
}

enum virtual_kind {
	GVARS_VIRTUAL_KIND_VIRTUAL,
	GVARS_VIRTUAL_KIND_INITIAL,
	GVARS_VIRTUAL_KIND_STATE,
};

static void
gvars_define_virtual_method(VALUE self, VALUE *name, VALUE backing, VALUE getter, VALUE setter, enum virtual_kind kind, int readonly_special)
{
	char *name_str = get_global_name(name);
	struct value_ptr *vp;
	VALUE vp_data = TypedData_Make_Struct(rb_cObject, struct value_ptr, &value_ptr_data_type, vp);
	vp->value = RB_ALLOC(VALUE);
	*vp->value = gvars_virtual_var_alloc(backing, getter, setter);

	rb_gvar_getter_t *getter_meth;
	rb_gvar_setter_t *setter_meth;

	switch (kind) {
	case GVARS_VIRTUAL_KIND_VIRTUAL:
		getter_meth = gvars_virtual_var_getter;
		setter_meth = setter == Qnil ? rb_gvar_readonly_setter : gvars_virtual_var_setter;
		break;

	case GVARS_VIRTUAL_KIND_STATE:
	case GVARS_VIRTUAL_KIND_INITIAL:
		getter_meth = kind == GVARS_VIRTUAL_KIND_INITIAL ? gvars_hooked_initial_var_getter : gvars_hooked_state_var_getter;
		if (readonly_special) setter_meth = rb_gvar_readonly_setter;
		else setter_meth = setter == Qnil ? gvars_hooked_var_setter_noproc : gvars_hooked_var_setter_proc;
		break;

	default:
		rb_bug("oops");
	}


	rb_define_hooked_variable(name_str, vp->value, getter_meth, setter_meth);

	VALUE hash = rb_iv_get(gvars_module, "vars");
	VALUE namesym = rb_str_new_cstr(name_str);
	rb_hash_aset(hash, namesym, vp_data);
}

enum virtual_method_keywords {
	VIRTUAL_METHOD_KEYWORDS_INITIAL,
	VIRTUAL_METHOD_KEYWORDS_STATE,
	VIRTUAL_METHOD_KEYWORDS_READONLY,
	LENGTH_OF_VIRTUAL_METHOD_KEYWORDS,
};

static ID virtual_method_keyword_ids[LENGTH_OF_VIRTUAL_METHOD_KEYWORDS];

static VALUE convert_to_proc(const char *name, VALUE input) {
	VALUE proc = rb_convert_type(input, T_DATA, "Proc", "to_proc");

	if (NIL_P(proc) || !rb_obj_is_proc(proc)) {
		rb_raise(rb_eTypeError, "wrong %s type %s (expected Proc)", name, rb_obj_classname(proc));
	}

	return proc;
}

static VALUE
gvars_f_define_virtual_method(int argc, VALUE *argv, VALUE self)
{
	VALUE name, backing, getter, setter, opts_hash, opts[LENGTH_OF_VIRTUAL_METHOD_KEYWORDS];

	switch (rb_scan_args(argc, argv, "12:", &name, &getter, &setter, &opts_hash)) {
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

	getter = convert_to_proc("getter", getter);
	if (!NIL_P(setter)) setter = convert_to_proc("setter", setter);

	enum virtual_kind kind = GVARS_VIRTUAL_KIND_VIRTUAL;
	rb_get_kwargs(opts_hash, virtual_method_keyword_ids, 0, LENGTH_OF_VIRTUAL_METHOD_KEYWORDS, opts);

	if (opts[VIRTUAL_METHOD_KEYWORDS_INITIAL] != Qundef) {
		if (opts[VIRTUAL_METHOD_KEYWORDS_STATE] != Qundef) {
			rb_raise(rb_eArgError, "only one of 'initial' or 'state' may be supplied");
		}
		backing = opts[VIRTUAL_METHOD_KEYWORDS_INITIAL];
		kind = GVARS_VIRTUAL_KIND_INITIAL;
	} else if (opts[VIRTUAL_METHOD_KEYWORDS_STATE] != Qundef) {
		backing = opts[VIRTUAL_METHOD_KEYWORDS_STATE];
		kind = GVARS_VIRTUAL_KIND_STATE;
	} else {
		backing = Qnil;
	}

 	// sadly no bool expected :-/
	bool is_readonly = opts[VIRTUAL_METHOD_KEYWORDS_READONLY] != Qundef && RTEST(opts[VIRTUAL_METHOD_KEYWORDS_READONLY]);
	if (is_readonly) {
		if (setter != Qnil) {
			rb_raise(rb_eArgError, "'readonly: true' may not be supplied when a setter proc is supplied");
		}

		setter = Qnil;
		is_readonly = true;
	}

	gvars_define_virtual_method(self, &name, backing, getter, setter, kind, is_readonly, passname);

	return name; //TODO: ID2SYM(id)
}

void
Init_gvars(void)
{
	gvars_module = rb_define_module("GVars");
	rb_ivar_set(gvars_module, rb_intern("vars"), rb_hash_new());

    virtual_method_keyword_ids[VIRTUAL_METHOD_KEYWORDS_INITIAL] = rb_intern("initial");
    virtual_method_keyword_ids[VIRTUAL_METHOD_KEYWORDS_STATE] = rb_intern("state");
    virtual_method_keyword_ids[VIRTUAL_METHOD_KEYWORDS_READONLY] = rb_intern("readonly");

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
