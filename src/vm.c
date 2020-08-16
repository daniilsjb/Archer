#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#include "vm.h"
#include "object.h"
#include "memory.h"

#if DEBUG_TRACE_EXECUTION
#include "debug.h"
#endif

static bool native_error(VM* vm, const char* message, Value* args)
{
    args[-1] = OBJ_VAL(copy_string(vm, message, strlen(message)));
    return false;
}

static bool clock_native(VM* vm, size_t argCount, Value* args)
{
    args[-1] = NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
    return true;
}

static bool abs_native(VM* vm, size_t argCount, Value* args)
{
    if (!IS_NUMBER(args[0])) {
        return native_error(vm, "Expected a numeric value.", args);
    }

    double number = AS_NUMBER(args[0]);
    args[-1] = NUMBER_VAL(fabs(number));
    return true;
}

static bool pow_native(VM* vm, size_t argCount, Value* args)
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
    table_put(vm, &vm->globals, AS_STRING(vm->stack[0]), vm->stack[1]);
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

    gc_init(&vm->gc);

    table_init(&vm->globals);
    table_init(&vm->strings);

    vm->initString = NULL;
    vm->initString = copy_string(vm, "init", 4);

    define_native(vm, "clock", clock_native, 0);
    define_native(vm, "abs", abs_native, 1);
    define_native(vm, "pow", pow_native, 2);
}

void vm_free(VM* vm)
{
    table_free(vm, &vm->globals);
    table_free(vm, &vm->strings);

    vm->initString = NULL;

    gc_free(&vm->gc, vm);

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

static InterpretStatus runtime_error(VM* vm, const char* format, ...)
{
    CallFrame* frame = &vm->frames[vm->frameCount - 1];
    ObjFunction* function = frame->closure->function;

    size_t instruction = frame->ip - frame->closure->function->chunk.code - 1;
    int line = chunk_get_line(&frame->closure->function->chunk, instruction);

    va_list args;
    va_start(args, format);
    fprintf(stderr, "[Line %d] ", line);
    vfprintf(stderr, format, args);
    fputs("\n", stderr);
    va_end(args);

    for (int i = (int)vm->frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm->frames[i];
        ObjFunction* function = frame->closure->function;

        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[Line %d] in ", chunk_get_line(&function->chunk, instruction));
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    reset_stack(vm);
    return INTERPRET_RUNTIME_ERROR;
}

static bool call(VM* vm, ObjClosure* closure, uint8_t argCount)
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
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
                vm->stackTop[- argCount - 1] = bound->receiver;
                return call(vm, bound->method, argCount);
            }
            case OBJ_CLOSURE: {
                return call(vm, AS_CLOSURE(callee), argCount);
            }
            case OBJ_NATIVE: {
                ObjNative* native = AS_NATIVE(callee);
                if (native->arity != argCount) {
                    runtime_error(vm, "Expected %d arguments but got %d", native->arity, argCount);
                    return false;
                }

                if (native->function(vm, argCount, vm->stackTop - argCount)) {
                    vm->stackTop -= (uint64_t)argCount;
                    return true;
                } else {
                    runtime_error(vm, AS_CSTRING(vm->stackTop[-argCount]));
                    return false;
                }
            }
            case OBJ_CLASS: {
                ObjClass* loxClass = AS_CLASS(callee);
                vm->stackTop[- argCount - 1] = OBJ_VAL(new_instance(vm, loxClass));

                Value initializer;
                if (table_get(&loxClass->methods, vm->initString, &initializer)) {
                    return call(vm, AS_CLOSURE(initializer), argCount);
                } else if (argCount != 0) {
                    runtime_error(vm, "Expected 0 arguments but got %d.", argCount);
                    return false;
                }
                return true;
            }
        }
    }

    runtime_error(vm, "Can only call functions and classes.");
    return false;
}

static bool invoke_from_class(VM* vm, ObjClass* loxClass, ObjString* name, uint8_t argCount)
{
    Value method;
    if (!table_get(&loxClass->methods, name, &method)) {
        runtime_error(vm, "Undefined property '%s'", name->chars);
        return false;
    }

    return call(vm, AS_CLOSURE(method), argCount);
}

static bool invoke(VM* vm, ObjString* name, uint8_t argCount)
{
    Value receiver = peek(vm, argCount);

    if (!IS_INSTANCE(receiver)) {
        runtime_error(vm, "Can only invoke methods on class instances.");
        return false;
    }

    ObjInstance* instance = AS_INSTANCE(receiver);

    Value value;
    if (table_get(&instance->fields, name, &value)) {
        vm->stackTop[- argCount - 1] = value;
        return call_value(vm, value, argCount);
    }

    return invoke_from_class(vm, instance->loxClass, name, argCount);
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

static void define_method(VM* vm, ObjString* name)
{
    Value method = peek(vm, 0);
    ObjClass* loxClass = AS_CLASS(peek(vm, 1));
    table_put(vm, &loxClass->methods, name, method);
    vm_pop(vm);
}

static bool bind_method(VM* vm, ObjClass* loxClass, ObjString* name)
{
    Value method;
    if (!table_get(&loxClass->methods, name, &method)) {
        runtime_error(vm, "Undefined property '%s'.", name->chars);
        return false;
    }

    ObjBoundMethod* bound = new_bound_method(vm, peek(vm, 0), AS_CLOSURE(method));
    vm_pop(vm);
    vm_push(vm, OBJ_VAL(bound));
    return true;
}

static InterpretStatus run(VM* vm)
{
    CallFrame* frame = &vm->frames[vm->frameCount - 1];
    register uint8_t* ip = frame->ip;

#define READ_BYTE() (*ip++)
#define READ_SHORT() (ip += 2, (uint16_t)(ip[-2] << 0 | ip[-1] << 8))
#define READ_CONSTANT() frame->closure->function->chunk.constants.values[READ_BYTE()]
#define READ_STRING() AS_STRING(READ_CONSTANT())

    while (true) {
#if DEBUG_TRACE_EXECUTION
        printf("\t");
        for (Value* slot = vm->stack; slot < vm->stackTop; slot++) {
            printf("[ ");
            print_value(*slot);
            printf(" ]");
        }
        printf("\n");

        disassemble_instruction(&frame->closure->function->chunk, (uint32_t)(ip - frame->closure->function->chunk.code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                vm_push(vm, READ_CONSTANT());
                break;
            }
            case OP_TRUE: {
                vm_push(vm, BOOL_VAL(true));
                break;
            }
            case OP_FALSE: {
                vm_push(vm, BOOL_VAL(false));
                break;
            }
            case OP_NIL: {
                vm_push(vm, NIL_VAL());
                break;
            }
            case OP_NOT_EQUAL: {
                Value b = vm_pop(vm);
                Value a = vm_pop(vm);
                vm_push(vm, BOOL_VAL(!values_equal(a, b)));
                break;
            }
            case OP_EQUAL: {
                Value b = vm_pop(vm);
                Value a = vm_pop(vm);
                vm_push(vm, BOOL_VAL(values_equal(a, b)));
                break;
            }
            case OP_GREATER: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    return runtime_error(vm, "Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop(vm));
                double a = AS_NUMBER(vm_pop(vm));
                vm_push(vm, BOOL_VAL(a > b));
                break;
            }
            case OP_GREATER_EQUAL: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    return runtime_error(vm, "Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop(vm));
                double a = AS_NUMBER(vm_pop(vm));
                vm_push(vm, BOOL_VAL(a >= b));
                break;
            }
            case OP_LESS: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    return runtime_error(vm, "Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop(vm));
                double a = AS_NUMBER(vm_pop(vm));
                vm_push(vm, BOOL_VAL(a < b));
                break;
            }
            case OP_LESS_EQUAL: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    return runtime_error(vm, "Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop(vm));
                double a = AS_NUMBER(vm_pop(vm));
                vm_push(vm, BOOL_VAL(a <= b));
                break;
            }
            case OP_NOT: {
                vm->stackTop[-1] = BOOL_VAL(value_is_falsey(vm->stackTop[-1]));
                break;
            }
            case OP_NEGATE: {
                if (!IS_NUMBER(peek(vm, 0))) {
                    return runtime_error(vm, "Operand must be a number.");
                }

                vm->stackTop[-1] = NUMBER_VAL(-AS_NUMBER(vm->stackTop[-1]));
                break;
            }
            case OP_ADD: {
                if (IS_STRING(peek(vm, 0)) && IS_STRING(peek(vm, 1))) {
                    ObjString* b = AS_STRING(peek(vm, 0));
                    ObjString* a = AS_STRING(peek(vm, 1));
                    ObjString* result = concatenate_strings(vm, a, b);

                    vm_pop(vm);
                    vm_pop(vm);
                    vm_push(vm, OBJ_VAL(result));
                } else if (IS_NUMBER(peek(vm, 0)) || IS_NUMBER(peek(vm, 1))) {
                    double b = AS_NUMBER(vm_pop(vm));
                    double a = AS_NUMBER(vm_pop(vm));
                    vm_push(vm, NUMBER_VAL(a + b));
                } else {
                    return runtime_error(vm, "Operands must be either numbers or strings.");
                }
                break;
            }
            case OP_SUBTRACT: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    return runtime_error(vm, "Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop(vm));
                double a = AS_NUMBER(vm_pop(vm));
                vm_push(vm, NUMBER_VAL(a - b));
                break;
            }
            case OP_MULTIPLY: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    return runtime_error(vm, "Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop(vm));
                double a = AS_NUMBER(vm_pop(vm));
                vm_push(vm, NUMBER_VAL(a * b));
                break;
            }
            case OP_DIVIDE: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    return runtime_error(vm, "Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop(vm));
                double a = AS_NUMBER(vm_pop(vm));
                vm_push(vm, NUMBER_VAL(a / b));
                break;
            }
            case OP_MODULO: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    return runtime_error(vm, "Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop(vm));
                double a = AS_NUMBER(vm_pop(vm));
                vm_push(vm, NUMBER_VAL(fmod(a, b)));
                break;
            }
            case OP_BITWISE_NOT: {
                if (!IS_NUMBER(peek(vm, 0))) {
                    return runtime_error(vm, "Operand must be a number.");
                }

                vm->stackTop[-1] = NUMBER_VAL((double)(~(int64_t)AS_NUMBER(vm->stackTop[-1])));
                break;
            }
            case OP_BITWISE_AND: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    return runtime_error(vm, "Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop(vm));
                double a = AS_NUMBER(vm_pop(vm));
                vm_push(vm, NUMBER_VAL((double)((int64_t)a & (int64_t)b)));
                break;
            }
            case OP_BITWISE_OR: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    return runtime_error(vm, "Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop(vm));
                double a = AS_NUMBER(vm_pop(vm));
                vm_push(vm, NUMBER_VAL((double)((int64_t)a | (int64_t)b)));
                break;
            }
            case OP_BITWISE_XOR: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    return runtime_error(vm, "Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop(vm));
                double a = AS_NUMBER(vm_pop(vm));
                vm_push(vm, NUMBER_VAL((double)((int64_t)a ^ (int64_t)b)));
                break;
            }
            case OP_BITWISE_LEFT_SHIFT: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    return runtime_error(vm, "Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop(vm));
                double a = AS_NUMBER(vm_pop(vm));
                vm_push(vm, NUMBER_VAL((double)((int64_t)a << (int64_t)b)));
                break;
            }
            case OP_BITWISE_RIGHT_SHIFT: {
                if (!IS_NUMBER(peek(vm, 0)) || !IS_NUMBER(peek(vm, 1))) {
                    return runtime_error(vm, "Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop(vm));
                double a = AS_NUMBER(vm_pop(vm));
                vm_push(vm, NUMBER_VAL((double)((int64_t)a >> (int64_t)b)));
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
                if (value_is_falsey(peek(vm, 0))) {
                    ip += offset;
                }
                break;
            }
            case OP_JUMP_IF_NOT_EQUAL: {
                uint16_t offset = READ_SHORT();
                if (!values_equal(peek(vm, 0), peek(vm, 1))) {
                    ip += offset;
                }
                break;
            }
            case OP_POP: {
                vm_pop(vm);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString* identifier = READ_STRING();
                table_put(vm, &vm->globals, identifier, peek(vm, 0));
                vm_pop(vm);
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* identifier = READ_STRING();
                if (table_put(vm, &vm->globals, identifier, peek(vm, 0))) {
                    frame->ip = ip;
                    table_remove(&vm->globals, identifier);
                    return runtime_error(vm, "Undefined variable '%s'.", identifier->chars);
                }
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* identifier = READ_STRING();
                Value value;
                if (!table_get(&vm->globals, identifier, &value)) {
                    frame->ip = ip;
                    return runtime_error(vm, "Undefined variable '%s'.", identifier->chars);
                }
                vm_push(vm, value);
                break;
            }
            case OP_SET_LOCAL: {
                frame->slots[READ_BYTE()] = peek(vm, 0);
                break;
            }
            case OP_GET_LOCAL: {
                vm_push(vm, frame->slots[READ_BYTE()]);
                break;
            }
            case OP_SET_UPVALUE: {
                *frame->closure->upvalues[READ_BYTE()]->location = peek(vm, 0);
                break;
            }
            case OP_GET_UPVALUE: {
                vm_push(vm, *frame->closure->upvalues[READ_BYTE()]->location);
                break;
            }
            case OP_SET_PROPERTY: {
                if (!IS_INSTANCE(peek(vm, 1))) {
                    runtime_error(vm, "Can only set properties of class instances.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = AS_INSTANCE(peek(vm, 1));
                table_put(vm, &instance->fields, READ_STRING(), peek(vm, 0));

                Value value = vm_pop(vm);
                vm_pop(vm);
                vm_push(vm, value);
                break;
            }
            case OP_GET_PROPERTY: {
                if (!IS_INSTANCE(peek(vm, 0))) {
                    runtime_error(vm, "Can only access properties of class instances.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = AS_INSTANCE(peek(vm, 0));
                ObjString* name = READ_STRING();

                Value value;
                if (table_get(&instance->fields, name, &value)) {
                    vm_pop(vm);
                    vm_push(vm, value);
                    break;
                }

                if (!bind_method(vm, instance->loxClass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                break;
            }
            case OP_PRINT: {
                print_value(vm_pop(vm));
                printf("\n");
                break;
            }
            case OP_CLOSURE: {
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure* closure = new_closure(vm, function);
                vm_push(vm, OBJ_VAL(closure));
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
                vm_pop(vm);
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
                Value result = vm_pop(vm);

                close_upvalues(vm, frame->slots);

                vm->frameCount--;
                if (vm->frameCount == 0) {
                    vm_pop(vm);
                    return INTERPRET_OK;
                }

                vm->stackTop = frame->slots;
                vm_push(vm, result);

                frame = &vm->frames[vm->frameCount - 1];
                ip = frame->ip;
                break;
            }
            case OP_CLASS: {
                vm_push(vm, OBJ_VAL(new_class(vm, READ_STRING())));
                break;
            }
            case OP_METHOD: {
                define_method(vm, READ_STRING());
                break;
            }
            case OP_INHERIT: {
                Value superclass = peek(vm, 1);
                if (!IS_CLASS(superclass)) {
                    return runtime_error(vm, "Superclass must be a class.");
                }

                ObjClass* subclass = AS_CLASS(peek(vm, 0));
                table_put_from(vm, &AS_CLASS(superclass)->methods, &subclass->methods);
                vm_pop(vm);
                break;
            }
            case OP_GET_SUPER: {
                ObjString* name = READ_STRING();
                ObjClass* superclass = AS_CLASS(vm_pop(vm));
                if (!bind_method(vm, superclass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUPER_INVOKE: {
                ObjString* method = READ_STRING();
                uint8_t argCount = READ_BYTE();
                ObjClass* superclass = AS_CLASS(vm_pop(vm));
                frame->ip = ip;

                if (!invoke_from_class(vm, superclass, method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
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
