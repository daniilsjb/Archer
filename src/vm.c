#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#include "vm.h"
#include "object.h"
#include "objstring.h"
#include "objnative.h"
#include "objfunction.h"
#include "common.h"
#include "memory.h"
#include "chunk.h"
#include "compiler.h"

#if DEBUG_TRACE_EXECUTION
#include "disassembler.h"
#endif

static bool native_error(VM* vm, const char* message, Value* args)
{
    args[-1] = OBJ_VAL(String_FromCString(vm, message));
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

static bool typeof_native(VM* vm, Value* args)
{
    if (!IS_OBJ(args[0])) {
        return native_error(vm, "Expected an object.", args);
    }

    args[-1] = OBJ_VAL(AS_OBJ(args[0])->type);
    return true;
}

static void define_native(VM* vm, const char* name, NativeFn function, int arity)
{
    vm_push(vm, OBJ_VAL(String_FromCString(vm, name)));
    vm_push(vm, OBJ_VAL(Native_New(vm, function, arity)));
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

    vm->initString = NULL;
    vm->stringType = NULL;
    vm->nativeType = NULL;
    vm->functionType = NULL;
    vm->upvalueType = NULL;
    vm->closureType = NULL;
    vm->boundMethodType = NULL;

    GC_Init(&vm->gc);
    vm->gc.vm = vm;

    table_init(&vm->globals);
    table_init(&vm->strings);

    vm->stringType = String_NewType(vm);
    vm->nativeType = Native_NewType(vm);
    vm->functionType = Function_NewType(vm);
    vm->upvalueType = Upvalue_NewType(vm);
    vm->closureType = Closure_NewType(vm);
    vm->boundMethodType = BoundMethod_NewType(vm);

    vm->initString = String_FromCString(vm, "init");

    String_PrepareType(vm->stringType, vm);
    Native_PrepareType(vm->nativeType, vm);
    Function_PrepareType(vm->functionType, vm);
    Upvalue_PrepareType(vm->upvalueType, vm);
    Closure_PrepareType(vm->closureType, vm);
    BoundMethod_PrepareType(vm->boundMethodType, vm);

    vm_push(vm, OBJ_VAL(String_FromCString(vm, "String")));
    table_put(vm, &vm->globals, String_FromCString(vm, "String"), OBJ_VAL(vm->stringType));
    vm_pop(vm);

    define_native(vm, "clock", clock_native, 0);
    define_native(vm, "abs", abs_native, 1);
    define_native(vm, "pow", pow_native, 2);
    define_native(vm, "typeOf", typeof_native, 1);
}

void vm_free(VM* vm)
{
    vm->initString = NULL;

    table_free(&vm->gc, &vm->globals);
    table_free(&vm->gc, &vm->strings);

    GC_Free(&vm->gc);

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
    ObjectFunction* function = frame->closure->function;
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

bool call(VM* vm, ObjectClosure* closure, uint8_t argCount)
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
    if (!OBJ_TYPE(object)->Call) {
        runtime_error(vm, "Objects of type '%s' are not callable.", OBJ_TYPE(object)->name);
        return false;
    }

    return Object_Call(object, argCount, vm);
}

static bool load_property(VM* vm, Object* object, ObjectString* name, Value* result)
{
    if (object->type->GetField) {
        if (Object_GetField(object, (Object*)name, vm, result)) {
            return true;
        }
    }

    if (!object->type->GetMethod) {
        runtime_error(vm, "Objects of type '%s' do not have methods.", object->type->name);
        return false;
    }

    if (!Object_GetMethod(object, (Object*)name, vm, result)) {
        runtime_error(vm, "Undefined property '%s'.", name->chars);
        return false;
    }

    return true;
}

static bool invoke_from_class(VM* vm, ObjectType* clazz, ObjectString* name, uint8_t argCount)
{
    Value method;
    if (!Object_GetMethodDirectly((Object*)clazz, (Object*)name, vm, &method)) {
        runtime_error(vm, "Undefined property '%s'", name->chars);
        return false;
    }

    return call_value(vm, method, argCount);
}

static bool invoke(VM* vm, ObjectString* name, uint8_t argCount)
{
    Value value = peek(vm, argCount);
    if (!IS_OBJ(value)) {
        runtime_error(vm, "Can only invoke methods on objects.");
        return false;
    }

    Object* receiver = AS_OBJ(value);
    Value method;
    if (!load_property(vm, receiver, name, &method)) {
        return false;
    }

    if (VAL_IS_BOUND_METHOD(method, vm)) {
        return Object_Call(AS_OBJ(method), argCount, vm);
    } else {
        vm->stackTop[-argCount - 1] = method;
        return call_value(vm, method, argCount);
    }
}

static bool invoke_static_constructor(VM* vm, ObjectType* clazz)
{
    Value method;
    if (Object_GetMethod((Object*)clazz, (Object*)vm->initString, vm, &method)) {
        return call_value(vm, method, 0);
    }

    return false;
}

static ObjectUpvalue* capture_upvalue(VM* vm, Value* local)
{
    ObjectUpvalue* prevUpvalue = NULL;
    ObjectUpvalue* upvalue = vm->openUpvalues;

    while (upvalue && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue && upvalue->location == local) {
        return upvalue;
    }

    ObjectUpvalue* createdUpvalue = Upvalue_New(vm, local);
    createdUpvalue->next = upvalue;

    if (!prevUpvalue) {
        vm->openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void close_upvalues(VM* vm, Value* last)
{
    while (vm->openUpvalues && vm->openUpvalues->location >= last) {
        ObjectUpvalue* upvalue = vm->openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm->openUpvalues = upvalue->next;
    }
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
                    ObjectString* b = VAL_AS_STRING(TOP);
                    ObjectString* a = VAL_AS_STRING(SND);
                    ObjectString* result = String_Concatenate(vm, a, b);

                    POP();
                    TOP = OBJ_VAL(result);
                } else if (IS_NUMBER(TOP) || IS_NUMBER(SND)) {
                    //TODO: Only perform this when both are numbers, but also ensure safety assignment still works
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
                ObjectString* identifier = READ_STRING();
                table_put(vm, &vm->globals, identifier, TOP);
                POP();
                break;
            }
            case OP_LOAD_GLOBAL: {
                ObjectString* identifier = READ_STRING();
                Value value;
                if (!table_get(&vm->globals, identifier, &value)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Undefined variable '%s'.", identifier->chars);
                }
                PUSH(value);
                break;
            }
            case OP_STORE_GLOBAL: {
                ObjectString* identifier = READ_STRING();
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
                Object* object = AS_OBJ(TOP);
                ObjectString* name = READ_STRING();

                frame->ip = ip;
                Value property;
                if (!load_property(vm, object, name, &property)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                TOP = property;
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
                if (!IS_OBJ(TOP)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Can only set properties of objects.");
                }

                Object* object = AS_OBJ(TOP);
                if (!object->type->SetField) {
                    frame->ip = ip;
                    return runtime_error(vm, "Properties on objects of type '%s' cannot be assigned.", object->type->name);
                }

                Object_SetField(object, (Object*)READ_STRING(), SND, vm);
                POP();
                break;
            }
            case OP_PRINT: {
                print_value(POP());
                printf("\n");
                break;
            }
            case OP_CLOSURE: {
                ObjectFunction* function = VAL_AS_FUNCTION(READ_CONSTANT());
                ObjectClosure* closure = Closure_New(vm, function);
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
                ObjectString* method = READ_STRING();
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
                PUSH(OBJ_VAL(Type_NewClass(vm, READ_STRING()->chars)));
                break;
            }
            case OP_STATIC_METHOD: {
                Value method = TOP;
                ObjectType* clazz = VAL_AS_TYPE(SND);
                Object_SetMethod((Object*)clazz, (Object*)READ_STRING(), method, vm);
                vm_pop(vm);
                break;
            }
            case OP_METHOD: {
                Value method = TOP;
                ObjectType* clazz = VAL_AS_TYPE(SND);
                Object_SetMethodDirectly((Object*)clazz, (Object*)READ_STRING(), method, vm);
                vm_pop(vm);
                break;
            }
            case OP_INHERIT: {
                if (!VAL_IS_TYPE(SND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Superclass must be a class.");
                }

                ObjectType* superclass = VAL_AS_TYPE(SND);
                if (!(superclass->flags & TF_ALLOW_INHERITANCE)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Class '%s' cannot be inherited from.", superclass->name);
                }

                ObjectType* subclass = VAL_AS_TYPE(TOP);
                table_put_from(vm, &superclass->methods, &subclass->methods);
                POP();
                break;
            }
            case OP_GET_SUPER: {
                ObjectString* name = READ_STRING();
                ObjectType* superclass = VAL_AS_TYPE(POP());

                Value method;
                if (!Object_GetMethod((Object*)superclass, (Object*)name, vm, &method)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Undefined method '%s' of superclass.", name->chars);
                }

                TOP = method;
                break;
            }
            case OP_SUPER_INVOKE: {
                ObjectString* name = READ_STRING();
                uint8_t argCount = READ_BYTE();
                ObjectType* superclass = VAL_AS_TYPE(POP());

                frame->ip = ip;
                if (!invoke_from_class(vm, superclass, name, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm->frames[vm->frameCount - 1];
                ip = frame->ip;
                break;
            }
            case OP_END_CLASS: {
                frame->ip = ip;
                if (!invoke_static_constructor(vm, VAL_AS_TYPE(TOP))) {
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
    ObjectFunction* function = compile(vm, source);
    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }

    vm_push(vm, OBJ_VAL(function));
    ObjectClosure* closure = Closure_New(vm, function);
    vm_pop(vm);
    vm_push(vm, OBJ_VAL(closure));
    call_value(vm, OBJ_VAL(closure), 0);

    return run(vm);
}
