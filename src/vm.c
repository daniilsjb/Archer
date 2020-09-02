#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#include "vm.h"
#include "object.h"
#include "objstring.h"
#include "objnative.h"
#include "objfunction.h"
#include "objclass.h"
#include "common.h"
#include "memory.h"
#include "chunk.h"
#include "compiler.h"

#if DEBUG_TRACE_EXECUTION
#include "disassembler.h"
#endif

static bool native_error(VM* vm, const char* message, Value* args)
{
    args[-1] = OBJ_VAL(copy_string(vm, message, strlen(message)));
    return false;
}

static bool clock_native(VM* vm, Value* args)
{
    args[-1] = NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
    return true;
}

static bool abs_native(VM* vm, Value* args)
{
    if (!IS_NUMBER(args[0])) {
        return native_error(vm, "Expected a numeric value.", args);
    }

    double number = AS_NUMBER(args[0]);
    args[-1] = NUMBER_VAL(fabs(number));
    return true;
}

static bool pow_native(VM* vm, Value* args)
{
    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        return native_error(vm, "Expected numeric values.", args);
    }

    double x = AS_NUMBER(args[0]);
    double y = AS_NUMBER(args[1]);
    args[-1] = NUMBER_VAL(pow(x, y));
    return true;
}

static void define_native(VM* vm, const char* name, NativeFn function, int arity)
{
    vm_push(vm, OBJ_VAL(copy_string(vm, name, strlen(name))));
    vm_push(vm, OBJ_VAL(new_native(vm, function, arity)));
    table_put(vm, &vm->globals, VAL_AS_STRING(vm->stack[0]), vm->stack[1]);
    vm_pop(vm);
    vm_pop(vm);
}

static void reset_stack(VM* vm)
{
    vm->stackTop = vm->stack;
    vm->frameCount = 0;
    vm->openUpvalues = NULL;
}

void vm_init(VM* vm)
{
    vm->compiler = NULL;
    vm->classCompiler = NULL;

    reset_stack(vm);

    vm->stringType = new_string_type(vm);
    vm->functionType = new_function_type(vm);
    vm->upvalueType = new_upvalue_type(vm);
    vm->closureType = new_closure_type(vm);
    vm->nativeType = new_native_type(vm);
    vm->instanceType = new_instance_type(vm);
    vm->classType = new_class_type(vm);
    vm->boundMethodType = new_bound_method_type(vm);

    gc_init(&vm->gc);
    vm->gc.vm = vm;

    table_init(&vm->globals);
    table_init(&vm->strings);

    vm->initString = copy_string(vm, "init", 4);

    define_native(vm, "clock", clock_native, 0);
    define_native(vm, "abs", abs_native, 1);
    define_native(vm, "pow", pow_native, 2);
}

void vm_free(VM* vm)
{
    table_free(&vm->gc, &vm->globals);
    table_free(&vm->gc, &vm->strings);

    vm->initString = NULL;

    gc_free(&vm->gc);

    free_bound_method_type(vm->boundMethodType, vm);
    free_class_type(vm->classType, vm);
    free_instance_type(vm->instanceType, vm);
    free_native_type(vm->nativeType, vm);
    free_closure_type(vm->closureType, vm);
    free_upvalue_type(vm->upvalueType, vm);
    free_function_type(vm->functionType, vm);
    free_string_type(vm->stringType, vm);

    reset_stack(vm);
}

void vm_push(VM* vm, Value value)
{
    *vm->stackTop = value;
    vm->stackTop++;
}

Value vm_pop(VM* vm)
{
    vm->stackTop--;
    return *vm->stackTop;
}

static Value peek(VM* vm, int distance)
{
    return vm->stackTop[- 1 - distance];
}

static int get_current_line(CallFrame* frame)
{
    Chunk* chunk = &frame->closure->function->chunk;
    size_t instruction = frame->ip - chunk->code - 1;
    return chunk_get_line(chunk, instruction);
}

static void print_call_frame(CallFrame* frame)
{
    ObjFunction* function = frame->closure->function;
    char* functionName = function->name ? function->name->chars : "script";
    fprintf(stderr, "[Line %d] in %s\n", get_current_line(frame), functionName);
}

static void print_stack_trace(VM* vm)
{
    for (size_t i = vm->frameCount - 1; i != (size_t)-1; i--) {
        print_call_frame(&vm->frames[i]);
    }
}

InterpretStatus runtime_error(VM* vm, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, "[Line %d] ", get_current_line(&vm->frames[vm->frameCount - 1]));
    vfprintf(stderr, format, args);
    fputs("\n", stderr);
    va_end(args);

    print_stack_trace(vm);

    reset_stack(vm);
    return INTERPRET_RUNTIME_ERROR;
}

bool call(VM* vm, ObjClosure* closure, uint8_t argCount)
{
    if (argCount != closure->function->arity) {
        runtime_error(vm, "Expected %d arguments but got %d", closure->function->arity, argCount);
        return false;
    }

    if (vm->frameCount == FRAMES_MAX) {
        runtime_error(vm, "Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm->frames[vm->frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm->stackTop - argCount - 1;
    return true;
}

static bool call_value(VM* vm, Value callee, uint8_t argCount)
{
    if (!IS_OBJ(callee)) {
        runtime_error(vm, "Can only call objects.");
        return false;
    }

    Object* object = AS_OBJ(callee);
    if (!OBJ_TYPE(object)->call) {
        runtime_error(vm, "Objects of type '%s' are not callable.", OBJ_TYPE(object)->name);
        return false;
    }

    return call_object(object, argCount, vm);
}

static bool invoke_from_class(VM* vm, ObjClass* loxClass, ObjString* name, uint8_t argCount)
{
    Value method;
    if (!table_get(&loxClass->methods, name, &method)) {
        runtime_error(vm, "Undefined property '%s'", name->chars);
        return false;
    }

    return call(vm, VAL_AS_CLOSURE(method), argCount);
}

static bool invoke(VM* vm, ObjString* name, uint8_t argCount)
{
    Value receiver = peek(vm, argCount);

    if (!VAL_IS_INSTANCE(receiver, vm)) {
        runtime_error(vm, "Can only invoke methods on class instances.");
        return false;
    }

    ObjInstance* instance = VAL_AS_INSTANCE(receiver);

    Value value;
    if (table_get(&instance->fields, name, &value)) {
        vm->stackTop[- argCount - 1] = value;
        return call_value(vm, value, argCount);
    }

    return invoke_from_class(vm, instance->clazz, name, argCount);
}

static ObjUpvalue* capture_upvalue(VM* vm, Value* local)
{
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = vm->openUpvalues;

    while (upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue* createdUpvalue = new_upvalue(vm, local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == NULL) {
        vm->openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void close_upvalues(VM* vm, Value* last)
{
    while (vm->openUpvalues != NULL && vm->openUpvalues->location >= last) {
        ObjUpvalue* upvalue = vm->openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm->openUpvalues = upvalue->next;
    }
}

static void define_static_method(VM* vm, ObjString* name)
{
    Value method = peek(vm, 0);
    ObjInstance* metaInstance = VAL_AS_INSTANCE(peek(vm, 1));
    table_put(vm, &metaInstance->clazz->methods, name, method);
    vm_pop(vm);
}

static void define_method(VM* vm, ObjString* name)
{
    Value method = peek(vm, 0);
    ObjClass* clazz = VAL_AS_CLASS(peek(vm, 1));
    table_put(vm, &clazz->methods, name, method);
    vm_pop(vm);
}

static bool bind_method(VM* vm, ObjClass* clazz, ObjString* name)
{
    Value method;
    if (!table_get(&clazz->methods, name, &method)) {
        runtime_error(vm, "Undefined property '%s'.", name->chars);
        return false;
    }

    ObjBoundMethod* bound = new_bound_method(vm, peek(vm, 0), VAL_AS_CLOSURE(method));
    vm_pop(vm);
    vm_push(vm, OBJ_VAL(bound));
    return true;
}

static bool invoke_static_constructor(VM* vm, ObjClass* loxClass)
{
    ObjInstance* metaInstance = (ObjInstance*)loxClass;

    Value method;
    if (table_get(&metaInstance->clazz->methods, vm->initString, &method)) {
        return call(vm, VAL_AS_CLOSURE(method), 0);
    }

    return false;
}

static InterpretStatus run(VM* vm)
{
    CallFrame* frame = &vm->frames[vm->frameCount - 1];
    register uint8_t* ip = frame->ip;

#define READ_BYTE() (*ip++)
#define READ_SHORT() (ip += 2, (uint16_t)(ip[-2] << 0 | ip[-1] << 8))
#define READ_CONSTANT() frame->closure->function->chunk.constants.data[READ_BYTE()]
#define READ_STRING() VAL_AS_STRING(READ_CONSTANT())

#define PEEK_BYTE() (*ip)
#define PEEK_NEXT_BYTE() (*(ip + 1))

#define SKIP_BYTE() (ip++)
#define SKIP_SHORT() (ip += 2)

#define AS_COMPLEMENT(value) ((int64_t)AS_NUMBER(value))

#define TOP vm->stackTop[-1]
#define SND vm->stackTop[-2]
#define THD vm->stackTop[-3]

#define PUSH(value) vm_push(vm, (value))
#define POP() vm_pop(vm)

    while (true) {
#if DEBUG_TRACE_EXECUTION
        printf("\t");
        for (Value* slot = vm->stack; slot < vm->stackTop; slot++) {
            printf("[ ");
            print_value(*slot);
            printf(" ]");
        }
        printf("\n");

        Chunk* chunk = &frame->closure->function->chunk;
        disassemble_instruction(chunk, (uint32_t)(ip - chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_LOAD_CONSTANT: {
                PUSH(READ_CONSTANT());
                break;
            }
            case OP_LOAD_TRUE: {
                PUSH(BOOL_VAL(true));
                break;
            }
            case OP_LOAD_FALSE: {
                PUSH(BOOL_VAL(false));
                break;
            }
            case OP_LOAD_NIL: {
                PUSH(NIL_VAL());
                break;
            }
            case OP_NOT_EQUAL: {
                Value rhs = POP();
                TOP = BOOL_VAL(!values_equal(TOP, rhs));
                break;
            }
            case OP_EQUAL: {
                Value rhs = POP();
                TOP = BOOL_VAL(values_equal(TOP, rhs));
                break;
            }
            case OP_GREATER: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                double rhs = AS_NUMBER(POP());
                TOP = BOOL_VAL(AS_NUMBER(TOP) > rhs);
                break;
            }
            case OP_GREATER_EQUAL: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                double rhs = AS_NUMBER(POP());
                TOP = BOOL_VAL(AS_NUMBER(TOP) >= rhs);
                break;
            }
            case OP_LESS: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                double rhs = AS_NUMBER(POP());
                TOP = BOOL_VAL(AS_NUMBER(TOP) < rhs);
                break;
            }
            case OP_LESS_EQUAL: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                double rhs = AS_NUMBER(POP());
                TOP = BOOL_VAL(AS_NUMBER(TOP) <= rhs);
                break;
            }
            case OP_NOT: {
                TOP = BOOL_VAL(value_is_falsey(TOP));
                break;
            }
            case OP_NEGATE: {
                if (!IS_NUMBER(TOP)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operand must be a number.");
                }

                TOP = NUMBER_VAL(-AS_NUMBER(TOP));
                break;
            }
            case OP_DEC: {
                if (!IS_NUMBER(TOP)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operand must be a number.");
                }

                TOP = NUMBER_VAL(AS_NUMBER(TOP) - 1);
                break;
            }
            case OP_INC: {
                if (!IS_NUMBER(TOP)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operand must be a number.");
                }

                TOP = NUMBER_VAL(AS_NUMBER(TOP) + 1);
                break;
            }
            case OP_ADD: {
                if (VAL_IS_STRING(TOP, vm) && VAL_IS_STRING(SND, vm)) {
                    ObjString* b = VAL_AS_STRING(TOP);
                    ObjString* a = VAL_AS_STRING(SND);
                    ObjString* result = concatenate_strings(vm, a, b);

                    POP();
                    TOP = OBJ_VAL(result);
                } else if (IS_NUMBER(TOP) || IS_NUMBER(SND)) {
                    double rhs = AS_NUMBER(POP());
                    TOP = NUMBER_VAL(AS_NUMBER(TOP) + rhs);
                } else {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be either numbers or strings.");
                }
                break;
            }
            case OP_SUBTRACT: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                double rhs = AS_NUMBER(POP());
                TOP = NUMBER_VAL(AS_NUMBER(TOP) - rhs);
                break;
            }
            case OP_MULTIPLY: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                double rhs = AS_NUMBER(POP());
                TOP = NUMBER_VAL(AS_NUMBER(TOP) * rhs);
                break;
            }
            case OP_DIVIDE: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                double rhs = AS_NUMBER(POP());
                TOP = NUMBER_VAL(AS_NUMBER(TOP) / rhs);
                break;
            }
            case OP_MODULO: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                double rhs = AS_NUMBER(POP());
                TOP = NUMBER_VAL(fmod(AS_NUMBER(TOP), rhs));
                break;
            }
            case OP_POWER: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                double exponent = AS_NUMBER(POP());
                TOP = NUMBER_VAL(pow(AS_NUMBER(TOP), exponent));
                break;
            }
            case OP_BITWISE_NOT: {
                if (!IS_NUMBER(TOP)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operand must be a number.");
                }

                TOP = NUMBER_VAL((double)(~AS_COMPLEMENT(TOP)));
                break;
            }
            case OP_BITWISE_AND: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                int64_t rhs = AS_COMPLEMENT(POP());
                TOP = NUMBER_VAL((double)(AS_COMPLEMENT(TOP) & rhs));
                break;
            }
            case OP_BITWISE_OR: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                int64_t rhs = AS_COMPLEMENT(POP());
                TOP = NUMBER_VAL((double)(AS_COMPLEMENT(TOP) | rhs));
                break;
            }
            case OP_BITWISE_XOR: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                int64_t rhs = AS_COMPLEMENT(POP());
                TOP = NUMBER_VAL((double)(AS_COMPLEMENT(TOP) ^ rhs));
                break;
            }
            case OP_BITWISE_LEFT_SHIFT: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                uint64_t rhs = AS_COMPLEMENT(POP());
                TOP = NUMBER_VAL((double)(AS_COMPLEMENT(TOP) << rhs));
                break;
            }
            case OP_BITWISE_RIGHT_SHIFT: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                uint64_t rhs = AS_COMPLEMENT(POP());
                TOP = NUMBER_VAL((double)(AS_COMPLEMENT(TOP) >> rhs));
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                ip -= offset;
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (value_is_falsey(TOP)) {
                    ip += offset;
                }
                break;
            }
            case OP_POP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (value_is_falsey(POP())) {
                    ip += offset;
                }
                break;
            }
            case OP_POP_JUMP_IF_EQUAL: {
                uint16_t offset = READ_SHORT();
                Value value = POP();
                if (values_equal(value, TOP)) {
                    ip += offset;
                }
                break;
            }
            case OP_JUMP_IF_NOT_NIL: {
                uint16_t offset = READ_SHORT();
                if (!IS_NIL(TOP)) {
                    ip += offset;
                }
                break;
            }
            case OP_POP: {
                POP();
                break;
            }
            case OP_DUP: {
                PUSH(TOP);
                break;
            }
            case OP_SWAP: {
                Value tmp = SND;
                SND = TOP;
                TOP = tmp;
                break;
            }
            case OP_SWAP_THREE: {
                Value oldThd = THD;
                THD = TOP;

                Value oldSnd = SND;
                SND = oldThd;

                TOP = oldSnd;
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString* identifier = READ_STRING();
                table_put(vm, &vm->globals, identifier, TOP);
                POP();
                break;
            }
            case OP_LOAD_GLOBAL: {
                ObjString* identifier = READ_STRING();
                Value value;
                if (!table_get(&vm->globals, identifier, &value)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Undefined variable '%s'.", identifier->chars);
                }
                PUSH(value);
                break;
            }
            case OP_STORE_GLOBAL: {
                ObjString* identifier = READ_STRING();
                if (table_put(vm, &vm->globals, identifier, TOP)) {
                    frame->ip = ip;
                    table_remove(&vm->globals, identifier);
                    return runtime_error(vm, "Undefined variable '%s'.", identifier->chars);
                }
                break;
            }
            case OP_LOAD_LOCAL: {
                PUSH(frame->slots[READ_BYTE()]);
                break;
            }
            case OP_STORE_LOCAL: {
                frame->slots[READ_BYTE()] = TOP;
                break;
            }
            case OP_LOAD_UPVALUE: {
                PUSH(*frame->closure->upvalues[READ_BYTE()]->location);
                break;
            }
            case OP_STORE_UPVALUE: {
                *frame->closure->upvalues[READ_BYTE()]->location = TOP;
                break;
            }
            case OP_LOAD_PROPERTY_SAFE: {
                if (IS_NIL(TOP)) {
                    SKIP_BYTE();
                    break;
                }
            }
            case OP_LOAD_PROPERTY: {
                if (!VAL_IS_INSTANCE(TOP, vm)) {
                    frame->ip = ip;
                    runtime_error(vm, "Can only access properties of class instances.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = VAL_AS_INSTANCE(TOP);
                ObjString* name = READ_STRING();

                Value value;
                if (table_get(&instance->fields, name, &value)) {
                    TOP = value;
                    break;
                }

                frame->ip = ip;
                if (!bind_method(vm, instance->clazz, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                break;
            }
            case OP_STORE_PROPERTY_SAFE: {
                if (IS_NIL(TOP)) {
                    SKIP_BYTE();
                    POP();
                    TOP = NIL_VAL();
                    break;
                }
            }
            case OP_STORE_PROPERTY: {
                if (!VAL_IS_INSTANCE(TOP, vm)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Can only set properties of class instances.");
                }

                ObjInstance* instance = VAL_AS_INSTANCE(TOP);
                table_put(vm, &instance->fields, READ_STRING(), SND);

                POP();
                break;
            }
            case OP_PRINT: {
                print_value(POP());
                printf("\n");
                break;
            }
            case OP_CLOSURE: {
                ObjFunction* function = VAL_AS_FUNCTION(READ_CONSTANT());
                ObjClosure* closure = new_closure(vm, function);
                PUSH(OBJ_VAL(closure));
                for (size_t i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues[i] = capture_upvalue(vm, frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case OP_CLOSE_UPVALUE: {
                close_upvalues(vm, vm->stackTop - 1);
                POP();
                break;
            }
            case OP_CALL: {
                uint8_t argCount = READ_BYTE();
                frame->ip = ip;

                frame->ip = ip;
                if (!call_value(vm, peek(vm, argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm->frames[vm->frameCount - 1];
                ip = frame->ip;
                break;
            }
            case OP_INVOKE_SAFE: {
                if (IS_NIL(peek(vm, PEEK_NEXT_BYTE()))) {
                    SKIP_BYTE();
                    vm->stackTop -= READ_BYTE();
                    break;
                }
            }
            case OP_INVOKE: {
                ObjString* method = READ_STRING();
                uint8_t argCount = READ_BYTE();

                frame->ip = ip;
                if (!invoke(vm, method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm->frames[vm->frameCount - 1];
                ip = frame->ip;
                break;
            }
            case OP_RETURN: {
                Value result = POP();

                close_upvalues(vm, frame->slots);

                vm->frameCount--;
                if (vm->frameCount == 0) {
                    POP();
                    return INTERPRET_OK;
                }

                vm->stackTop = frame->slots;
                PUSH(result);

                frame = &vm->frames[vm->frameCount - 1];
                ip = frame->ip;
                break;
            }
            case OP_CLASS: {
                PUSH(OBJ_VAL(new_class(vm, READ_STRING())));
                break;
            }
            case OP_STATIC_METHOD: {
                define_static_method(vm, READ_STRING());
                break;
            }
            case OP_METHOD: {
                define_method(vm, READ_STRING());
                break;
            }
            case OP_INHERIT: {
                Value superclass = SND;
                if (!VAL_IS_CLASS(superclass, vm)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Superclass must be a class.");
                }

                ObjClass* subclass = VAL_AS_CLASS(TOP);
                table_put_from(vm, &VAL_AS_CLASS(superclass)->methods, &subclass->methods);
                POP();
                break;
            }
            case OP_GET_SUPER: {
                ObjString* name = READ_STRING();
                ObjClass* superclass = VAL_AS_CLASS(POP());

                frame->ip = ip;
                if (!bind_method(vm, superclass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUPER_INVOKE: {
                ObjString* method = READ_STRING();
                uint8_t argCount = READ_BYTE();
                ObjClass* superclass = VAL_AS_CLASS(POP());

                frame->ip = ip;
                if (!invoke_from_class(vm, superclass, method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm->frames[vm->frameCount - 1];
                ip = frame->ip;
                break;
            }
            case OP_END_CLASS: {
                frame->ip = ip;
                if (!invoke_static_constructor(vm, VAL_AS_CLASS(TOP))) {
                    POP();
                }
                frame = &vm->frames[vm->frameCount - 1];
                ip = frame->ip;
                break;
            }
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING

#undef PEEK_BYTE
#undef PEEK_NEXT_BYTE

#undef SKIP_BYTE
#undef SKIP_SHORT

#undef AS_COMPLEMENT

#undef TOP
#undef SND
#undef THD

#undef PUSH
#undef POP
}

InterpretStatus vm_interpret(VM* vm, const char* source)
{
    ObjFunction* function = compile(vm, source);
    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }

    vm_push(vm, OBJ_VAL(function));
    ObjClosure* closure = new_closure(vm, function);
    vm_pop(vm);
    vm_push(vm, OBJ_VAL(closure));
    call_value(vm, OBJ_VAL(closure), 0);

    return run(vm);
}
