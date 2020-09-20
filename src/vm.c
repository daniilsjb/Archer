#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#include "vm.h"
#include "common.h"
#include "memory.h"
#include "chunk.h"
#include "compiler.h"
#include "library.h"

#include "object.h"
#include "objstring.h"
#include "objnative.h"
#include "objfunction.h"
#include "objcoroutine.h"
#include "objlist.h"
#include "objmap.h"

#if DEBUG_TRACE_EXECUTION
#include "disassembler.h"
#endif

void vm_init(VM* vm)
{
    vm->compiler = NULL;
    vm->classCompiler = NULL;

    vm->coroutine = NULL;

    vm->initString = NULL;

    vm->stringType = NULL;
    vm->nativeType = NULL;
    vm->functionType = NULL;
    vm->upvalueType = NULL;
    vm->closureType = NULL;
    vm->boundMethodType = NULL;
    vm->coroutineType = NULL;
    vm->listType = NULL;
    vm->mapType = NULL;
    vm->arrayType = NULL;

    GC_Init(&vm->gc);
    vm->gc.vm = vm;

    table_init(&vm->globals);
    table_init(&vm->strings);

    vm->temporaryCount = 0;

    Library_Init(vm);
}

void vm_free(VM* vm)
{
    vm->initString = NULL;

    table_free(&vm->gc, &vm->globals);
    table_free(&vm->gc, &vm->strings);

    GC_Free(&vm->gc);
}

void vm_push(VM* vm, Value value)
{
    *vm->coroutine->stackTop = value;
    vm->coroutine->stackTop++;
}

Value vm_pop(VM* vm)
{
    vm->coroutine->stackTop--;
    return *vm->coroutine->stackTop;
}

Value vm_peek(VM* vm, int distance)
{
    return vm->coroutine->stackTop[-1 - distance];
}

void vm_push_temporary(VM* vm, Value value)
{
    vm->temporaries[vm->temporaryCount++] = value;
}

Value vm_pop_temporary(VM* vm)
{
    vm->temporaryCount--;
    return vm->temporaries[vm->temporaryCount];
}

Value vm_peek_temporary(VM* vm, int distance)
{
    return vm->temporaries[vm->temporaryCount - 1 - distance];
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
    for (size_t i = vm->coroutine->frameCount - 1; i != (size_t)-1; i--) {
        print_call_frame(&vm->coroutine->frames[i]);
    }
}

InterpretStatus runtime_error(VM* vm, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, "[Line %d] ", get_current_line(&vm->coroutine->frames[vm->coroutine->frameCount - 1]));
    vfprintf(stderr, format, args);
    fputs("\n", stderr);
    va_end(args);

    print_stack_trace(vm);

    //reset_stack(vm);
    return INTERPRET_RUNTIME_ERROR;
}

bool call(VM* vm, ObjectClosure* closure, uint8_t argCount)
{
    if (argCount != closure->function->arity) {
        runtime_error(vm, "Expected %d arguments but got %d", closure->function->arity, argCount);
        return false;
    }

    if (vm->coroutine->frameCount == FRAMES_MAX) {
        runtime_error(vm, "Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm->coroutine->frames[vm->coroutine->frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm->coroutine->stackTop - argCount - 1;
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
    Value key = OBJ_VAL(name);

    if (object->type->GetField) {
        if (Object_GetField(object, key, vm, result)) {
            return true;
        }
    }

    if (!object->type->GetMethod) {
        runtime_error(vm, "Objects of type '%s' do not have methods.", object->type->name);
        return false;
    }

    if (!Object_GetMethod(object, key, vm, result)) {
        runtime_error(vm, "Undefined property '%s'.", name->chars);
        return false;
    }

    return true;
}

static bool invoke_from_class(VM* vm, ObjectType* clazz, ObjectString* name, uint8_t argCount)
{
    Value key = OBJ_VAL(name);
    Value method;
    if (!Object_GetMethodDirectly((Object*)clazz, key, vm, &method)) {
        runtime_error(vm, "Undefined property '%s'", name->chars);
        return false;
    }

    return call_value(vm, method, argCount);
}

static bool invoke(VM* vm, ObjectString* name, uint8_t argCount)
{
    Value value = vm_peek(vm, argCount);
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
        vm->coroutine->stackTop[-argCount - 1] = method;
        return call_value(vm, method, argCount);
    }
}

static bool invoke_static_constructor(VM* vm, ObjectType* clazz)
{
    Value key = OBJ_VAL(vm->initString);
    Value method;
    if (Object_GetMethod((Object*)clazz, key, vm, &method)) {
        return call_value(vm, method, 0);
    }

    return false;
}

static ObjectUpvalue* capture_upvalue(VM* vm, Value* local)
{
    ObjectUpvalue* prevUpvalue = NULL;
    ObjectUpvalue* upvalue = vm->coroutine->openUpvalues;

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
        vm->coroutine->openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void close_upvalues(VM* vm, Value* last)
{
    while (vm->coroutine->openUpvalues && vm->coroutine->openUpvalues->location >= last) {
        ObjectUpvalue* upvalue = vm->coroutine->openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm->coroutine->openUpvalues = upvalue->next;
    }
}

static InterpretStatus run(VM* vm)
{
    register ObjectCoroutine* coroutine = vm->coroutine;
    register CallFrame* frame = &coroutine->frames[coroutine->frameCount - 1];
    register uint8_t* ip = frame->ip;

#define UPDATE_POINTERS()                                       \
    do {                                                        \
        coroutine = vm->coroutine;                              \
        frame = &coroutine->frames[coroutine->frameCount - 1];  \
        ip = frame->ip;                                         \
    } while (0)                                                 \

#define READ_BYTE() (*ip++)
#define READ_SHORT() (ip += 2, (uint16_t)(ip[-2] << 0 | ip[-1] << 8))
#define READ_CONSTANT() frame->closure->function->chunk.constants.data[READ_BYTE()]
#define READ_STRING() VAL_AS_STRING(READ_CONSTANT())

#define PEEK_BYTE() (*ip)
#define PEEK_NEXT_BYTE() (*(ip + 1))

#define SKIP_BYTE() (ip++)
#define SKIP_SHORT() (ip += 2)

#define AS_COMPLEMENT(value) ((int64_t)AS_NUMBER(value))

#define TOP    coroutine->stackTop[-1]
#define SECOND coroutine->stackTop[-2]
#define THIRD  coroutine->stackTop[-3]
#define FOURTH coroutine->stackTop[-4]

#define PUSH(value) vm_push(vm, (value))
#define POP() vm_pop(vm)
#define PEEK(distance) vm_peek(vm, distance)

#define POP_N(n) coroutine->stackTop -= (n);

    while (true) {
#if DEBUG_TRACE_EXECUTION
        printf("\t");
        for (Value* slot = coroutine->stack; slot < coroutine->stackTop; slot++) {
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
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SECOND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                double rhs = AS_NUMBER(POP());
                TOP = BOOL_VAL(AS_NUMBER(TOP) > rhs);
                break;
            }
            case OP_GREATER_EQUAL: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SECOND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                double rhs = AS_NUMBER(POP());
                TOP = BOOL_VAL(AS_NUMBER(TOP) >= rhs);
                break;
            }
            case OP_LESS: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SECOND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                double rhs = AS_NUMBER(POP());
                TOP = BOOL_VAL(AS_NUMBER(TOP) < rhs);
                break;
            }
            case OP_LESS_EQUAL: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SECOND)) {
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
                if (VAL_IS_STRING(TOP, vm) && VAL_IS_STRING(SECOND, vm)) {
                    ObjectString* b = VAL_AS_STRING(TOP);
                    ObjectString* a = VAL_AS_STRING(SECOND);
                    ObjectString* result = String_Concatenate(vm, a, b);

                    POP();
                    TOP = OBJ_VAL(result);
                } else if (IS_NUMBER(TOP) && IS_NUMBER(SECOND)) {
                    double rhs = AS_NUMBER(POP());
                    TOP = NUMBER_VAL(AS_NUMBER(TOP) + rhs);
                } else {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be either numbers or strings.");
                }
                break;
            }
            case OP_SUBTRACT: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SECOND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                double rhs = AS_NUMBER(POP());
                TOP = NUMBER_VAL(AS_NUMBER(TOP) - rhs);
                break;
            }
            case OP_MULTIPLY: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SECOND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                double rhs = AS_NUMBER(POP());
                TOP = NUMBER_VAL(AS_NUMBER(TOP) * rhs);
                break;
            }
            case OP_DIVIDE: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SECOND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                double rhs = AS_NUMBER(POP());
                TOP = NUMBER_VAL(AS_NUMBER(TOP) / rhs);
                break;
            }
            case OP_MODULO: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SECOND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                double rhs = AS_NUMBER(POP());
                TOP = NUMBER_VAL(fmod(AS_NUMBER(TOP), rhs));
                break;
            }
            case OP_POWER: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SECOND)) {
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
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SECOND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                int64_t rhs = AS_COMPLEMENT(POP());
                TOP = NUMBER_VAL((double)(AS_COMPLEMENT(TOP) & rhs));
                break;
            }
            case OP_BITWISE_OR: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SECOND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                int64_t rhs = AS_COMPLEMENT(POP());
                TOP = NUMBER_VAL((double)(AS_COMPLEMENT(TOP) | rhs));
                break;
            }
            case OP_BITWISE_XOR: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SECOND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                int64_t rhs = AS_COMPLEMENT(POP());
                TOP = NUMBER_VAL((double)(AS_COMPLEMENT(TOP) ^ rhs));
                break;
            }
            case OP_BITWISE_LEFT_SHIFT: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SECOND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Operands must be numbers");
                }

                uint64_t rhs = AS_COMPLEMENT(POP());
                TOP = NUMBER_VAL((double)(AS_COMPLEMENT(TOP) << rhs));
                break;
            }
            case OP_BITWISE_RIGHT_SHIFT: {
                if (!IS_NUMBER(TOP) || !IS_NUMBER(SECOND)) {
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
            case OP_POP_LOOP_IF_TRUE: {
                uint16_t offset = READ_SHORT();
                if (!value_is_falsey(POP())) {
                    ip -= offset;
                }
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
                if (values_equal(TOP, SECOND)) {
                    ip += offset;
                }
                POP();
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
            case OP_DUP_TWO: {
                PUSH(SECOND);
                PUSH(SECOND);
                break;
            }
            case OP_SWAP: {
                Value tmp = SECOND;
                SECOND = TOP;
                TOP = tmp;
                break;
            }
            case OP_SWAP_THREE: {
                Value third = THIRD;
                THIRD = TOP;

                Value second = SECOND;
                SECOND = third;

                TOP = second;
                break;
            }
            case OP_SWAP_FOUR: {
                Value fourth = FOURTH;
                FOURTH = TOP;

                Value third = THIRD;
                THIRD = fourth;

                Value second = SECOND;
                SECOND = third;

                TOP = second;
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjectString* identifier = READ_STRING();
                table_put(vm, &vm->globals, OBJ_VAL(identifier), TOP);
                POP();
                break;
            }
            case OP_LOAD_GLOBAL: {
                ObjectString* identifier = READ_STRING();
                Value key = OBJ_VAL(identifier);
                Value value;
                if (!table_get(&vm->globals, key, &value)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Undefined variable '%s'.", identifier->chars);
                }
                PUSH(value);
                break;
            }
            case OP_STORE_GLOBAL: {
                ObjectString* identifier = READ_STRING();
                Value key = OBJ_VAL(identifier);
                if (table_put(vm, &vm->globals, key, TOP)) {
                    frame->ip = ip;
                    table_remove(&vm->globals, key);
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

                Object_SetField(object, OBJ_VAL(READ_STRING()), SECOND, vm);
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
                close_upvalues(vm, coroutine->stackTop - 1);
                POP();
                break;
            }
            case OP_CALL: {
                uint8_t argCount = READ_BYTE();

                frame->ip = ip;
                if (!call_value(vm, PEEK(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                UPDATE_POINTERS();
                break;
            }
            case OP_INVOKE_SAFE: {
                if (IS_NIL(PEEK(PEEK_NEXT_BYTE()))) {
                    SKIP_BYTE();
                    POP_N(READ_BYTE());
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

                UPDATE_POINTERS();
                break;
            }
            case OP_RETURN: {
                Value result = POP();

                close_upvalues(vm, frame->slots);
                coroutine->stackTop = frame->slots;

                coroutine->frameCount--;
                if (coroutine->frameCount == 0) {
                    vm->coroutine = coroutine->transfer;
                    if (!vm->coroutine) {
                        return INTERPRET_OK;
                    }
                }

                UPDATE_POINTERS();
                PUSH(result);
                break;
            }
            case OP_CLASS: {
                PUSH(OBJ_VAL(Type_NewClass(vm, READ_STRING()->chars)));
                break;
            }
            case OP_STATIC_METHOD: {
                Value method = TOP;
                ObjectType* clazz = VAL_AS_TYPE(SECOND);
                Object_SetMethod((Object*)clazz, OBJ_VAL(READ_STRING()), method, vm);
                vm_pop(vm);
                break;
            }
            case OP_METHOD: {
                Value method = TOP;
                ObjectType* clazz = VAL_AS_TYPE(SECOND);
                Object_SetMethodDirectly((Object*)clazz, OBJ_VAL(READ_STRING()), method, vm);
                vm_pop(vm);
                break;
            }
            case OP_INHERIT: {
                if (!VAL_IS_TYPE(SECOND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Superclass must be a class.");
                }

                ObjectType* superclass = VAL_AS_TYPE(SECOND);
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

                Value key = OBJ_VAL(name);
                Value method;
                if (!Object_GetMethod((Object*)superclass, key, vm, &method)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Undefined method '%s' of superclass.", name->chars);
                }

                TOP = key;
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

                UPDATE_POINTERS();
                break;
            }
            case OP_END_CLASS: {
                frame->ip = ip;
                if (!invoke_static_constructor(vm, VAL_AS_TYPE(TOP))) {
                    POP();
                }

                UPDATE_POINTERS();
                break;
            }
            case OP_LOAD_SUBSCRIPT_SAFE: {
                if (IS_NIL(SECOND)) {
                    POP();
                    TOP = NIL_VAL();
                    break;
                }
            }
            case OP_LOAD_SUBSCRIPT: {
                if (!IS_OBJ(SECOND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Can only subscript objects.");
                }

                Object* object = AS_OBJ(SECOND);

                if (!object->type->GetSubscript) {
                    frame->ip = ip;
                    return runtime_error(vm, "Objects of type '%s' cannot be subscripted.", object->type->name);
                }

                Value result;
                frame->ip = ip;
                if (!Object_GetSubscript(object, TOP, vm, &result)) {
                    return false;
                }

                POP();
                TOP = result;
                break;
            }
            case OP_STORE_SUBSCRIPT_SAFE: {
                if (IS_NIL(SECOND)) {
                    POP();
                    TOP = NIL_VAL();
                    break;
                }
            }
            case OP_STORE_SUBSCRIPT: {
                if (!IS_OBJ(SECOND)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Can only subscript objects.");
                }

                Object* object = AS_OBJ(SECOND);

                if (!object->type->SetSubscript) {
                    frame->ip = ip;
                    return runtime_error(vm, "Objects of type '%s' cannot be subscripted.", object->type->name);
                }

                frame->ip = ip;
                if (!Object_SetSubscript(object, TOP, THIRD, vm)) {
                    return false;
                }

                POP_N(2);
                break;
            }
            case OP_LIST: {
                uint8_t count = READ_BYTE();

                if (count == 0) {
                    PUSH(OBJ_VAL(List_New(vm)));
                    break;
                }

                Value* accumulator = coroutine->stackTop - count;
                PUSH(*accumulator);

                *accumulator = OBJ_VAL(List_New(vm));
                List_Append(VAL_AS_LIST(*accumulator), TOP, vm);
                POP();

                for (Value* value = accumulator + 1; value < coroutine->stackTop; value++) {
                    List_Append(VAL_AS_LIST(*accumulator), *value, vm);
                }

                POP_N((size_t)count - 1);
                break;
            }
            case OP_MAP: {
                uint8_t entryCount = READ_BYTE();
                uint16_t count = ((uint16_t)entryCount * 2);

                if (entryCount == 0) {
                    PUSH(OBJ_VAL(Map_New(vm)));
                    break;
                }

                Value* accumulator = coroutine->stackTop - count;
                PUSH(*accumulator);

                *accumulator = OBJ_VAL(Map_New(vm));
                Map_Insert(VAL_AS_MAP(*accumulator), TOP, *(accumulator + 1), vm);
                POP();

                for (Value* value = accumulator + 2; value < coroutine->stackTop; value += 2) {
                    Map_Insert(VAL_AS_MAP(*accumulator), *value, *(value + 1), vm);
                }

                POP_N((size_t)count - 1);
                break;
            }
            case OP_BUILD_STRING: {
                uint8_t count = READ_BYTE();
                
                Value* accumulator = coroutine->stackTop - count;
                *accumulator = OBJ_VAL(String_FromValue(vm, *accumulator));

                for (Value* current = coroutine->stackTop - count + 1; current < coroutine->stackTop; current++) {
                    *current = OBJ_VAL(String_FromValue(vm, *current));
                    *accumulator = OBJ_VAL(String_Concatenate(vm, VAL_AS_STRING(*accumulator), VAL_AS_STRING(*current)));
                }

                POP_N((size_t)count - 1);
                break;
            }
            case OP_YIELD: {
                Value result = POP();
                close_upvalues(vm, frame->slots);

                if (!coroutine->transfer) {
                    frame->ip = ip;
                    return runtime_error(vm, "Cannot yield outside a coroutine.");
                }

                coroutine->frames[coroutine->frameCount - 1].ip = ip;
                vm->coroutine = coroutine->transfer;

                UPDATE_POINTERS();
                PUSH(result);
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
#undef SECOND
#undef THIRD
#undef FOURTH

#undef PUSH
#undef POP
}

InterpretStatus vm_interpret(VM* vm, const char* source)
{
    ObjectFunction* function = compile(vm, source);
    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }
    
    vm_push_temporary(vm, OBJ_VAL(function));
    ObjectClosure* closure = Closure_New(vm, function);
    vm_pop_temporary(vm);

    vm_push_temporary(vm, OBJ_VAL(closure));
    _Coroutine_CallMain(vm, Coroutine_New(vm, closure));
    vm_pop_temporary(vm);

    return run(vm);
}
