#include "module.h"
#include "thread/threads.h"

typedef struct {
  b_value *values;
  int arg_count;
} BThreadParam;

typedef struct {
  b_vm *vm;
  int function_type;
  int callback_type;
  thrd_t thread;
  BThreadParam *params;
  union {
    b_obj_closure *closure;
    b_obj_native *native;
  } function;
  union {
    b_obj_closure *closure;
    b_obj_native *native;
  } callback;
} BThread;

b_vm *clone_vm(b_vm *vm) {
  b_vm *new_vm = (b_vm *) malloc(sizeof(b_vm));
  if(new_vm != NULL) {
    memset(vm, 0, sizeof(b_vm));
    bind_native_modules(vm);
    init_vm(new_vm);
    table_add_all(new_vm, &vm->modules, new_vm->modules);
    new_vm->should_debug_stack = vm->should_debug_stack;
    new_vm->should_print_bytecode = vm->should_print_bytecode;
    new_vm->is_repl = vm->is_repl;
  }
  return new_vm;
}

int threading_function(void *data) {
  BThread *thread = (BThread*)data;
}

DECLARE_MODULE_METHOD(thread_new) {
  ENFORCE_ARG_COUNT(new, 0);
  BThread *thread = (BThread*) malloc(sizeof(BThread));
  memset(thread, 0, sizeof(BThread));
  RETURN_PTR(thread);
}

DECLARE_MODULE_METHOD(thread_set_function) {
  ENFORCE_ARG_COUNT(set_function, 2);
  ENFORCE_ARG_TYPE(set_function, 0, IS_PTR);
  BThread *thread = (BThread*) AS_PTR(args[0])->pointer;

  if(IS_CLOSURE(arg[1])) {
    thread->function_type = TYPE_FUNCTION;
    thread->function.closure = AS_CLOSURE(args[1]);
  } else if(IS_NATIVE(args[1])) {
    thread->function_type = TYPE_SCRIPT;
    thread->function.native = AS_NATIVE(args[1]);
  } else {
    RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_MODULE_METHOD(thread_set_callback) {
  ENFORCE_ARG_COUNT(set_callback, 2);
  ENFORCE_ARG_TYPE(set_callback, 0, IS_PTR);
  BThread *thread = (BThread*) AS_PTR(args[0])->pointer;

  if(IS_CLOSURE(arg[1])) {
    thread->callback_type = TYPE_FUNCTION;
    thread->callback.closure = AS_CLOSURE(args[1]);
  } else if(IS_NATIVE(args[1])) {
    thread->callback_type = TYPE_SCRIPT;
    thread->callback.native = AS_NATIVE(args[1]);
  } else {
    RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_MODULE_METHOD(thread_set_params) {
  ENFORCE_ARG_COUNT(set_params, 2);
  ENFORCE_ARG_TYPE(set_callback, 0, IS_PTR);
  ENFORCE_ARG_TYPE(set_callback, 1, IS_LIST);
  BThread *thread = (BThread*) AS_PTR(args[0])->pointer;
  b_obj_list *list = AS_LIST(args[1]);

  BThreadParam *params = (BThreadParam*) calloc(1, sizeof(BThreadParam));
  params->values = list->items.values;
  params->arg_count = list->items.count;

  thread->params = params;

  RETURN;
}

DECLARE_MODULE_METHOD(thread_run) {
  ENFORCE_ARG_COUNT(run, 1);
  BThread *thread = (BThread*) AS_PTR(args[0])->pointer;

  b_vm *new_vm = clone_vm(vm);
  thread->vm = new_vm;

  RETURN_NUMBER(thrd_create(&thread->thread, &threading_function, (void *)thread));
}

CREATE_MODULE_LOADER(base64) {
  static b_func_reg module_functions[] = {
      {NULL,     false, NULL},
 };

  static b_module_reg module = {
      .name = "_thread",
      .fields = NULL,
      .functions = module_functions,
      .classes = NULL,
      .preloader = NULL,
      .unloader = NULL
  };

  return &module;
}