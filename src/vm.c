#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#include "vm.h"
#include "compiler.h"
#include "object.h"
#include "memory.h"

#if DEBUG_TRACE_EXECUTION
#include "debug.h"
#endif

VM vm;

static bool native_error(const char* message, Value* args)
{
    args[-1] = OBJ_VAL(copy_string(message, strlen(message)));
    return false;
}

static bool clock_native(size_t argCount, Value* args)
{
    args[-1] = NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
    return true;
}

static bool abs_native(size_t argCount, Value* args)
{
    if (!IS_NUMBER(args[0])) {
        return native_error("Expected a numeric value.", args);
    }

    double number = AS_NUMBER(args[0]);
    args[-1] = NUMBER_VAL(fabs(number));
    return true;
}

static bool pow_native(size_t argCount, Value* args)
{
    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        return native_error("Expected numeric values.", args);
    }

    double x = AS_NUMBER(args[0]);
    double y = AS_NUMBER(args[1]);
    args[-1] = NUMBER_VAL(pow(x, y));
    return true;
}

static void define_native(const char* name, NativeFn function, int arity)
{
    vm_push(OBJ_VAL(copy_string(name, strlen(name))));
    vm_push(OBJ_VAL(new_native(function, arity)));
    table_put(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    vm_pop();
    vm_pop();
}

static void reset_stack()
{
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
}

void vm_init()
{
    reset_stack();

    vm.objects = NULL;

    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;

    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;

    table_init(&vm.globals);
    table_init(&vm.strings);

    vm.initString = NULL;
    vm.initString = copy_string("init", 4);

    define_native("clock", clock_native, 0);
    define_native("abs", abs_native, 1);
    define_native("pow", pow_native, 2);
}

void vm_free()
{
    table_free(&vm.globals);
    table_free(&vm.strings);

    vm.initString = NULL;

    free_objects();

    reset_stack();
}

void vm_push(Value value)
{
    *vm.stackTop = value;
    vm.stackTop++;
}

Value vm_pop()
{
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek(int distance)
{
    return vm.stackTop[- 1 - distance];
}

static bool is_falsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static InterpretStatus runtime_error(const char* format, ...)
{
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    ObjFunction* function = frame->closure->function;

    size_t instruction = frame->ip - frame->closure->function->chunk.code - 1;
    int line = chunk_get_line(&frame->closure->function->chunk, instruction);

    va_list args;
    va_start(args, format);
    fprintf(stderr, "[Line %d] ", line);
    vfprintf(stderr, format, args);
    fputs("\n", stderr);
    va_end(args);

    for (int i = (int)vm.frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->closure->function;

        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[Line %d] in ", chunk_get_line(&function->chunk, instruction));
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    reset_stack();
    return INTERPRET_RUNTIME_ERROR;
}

static bool call(ObjClosure* closure, uint8_t argCount)
{
    if (argCount != closure->function->arity) {
        runtime_error("Expected %d arguments but got %d", closure->function->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX) {
        runtime_error("Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

static bool call_value(Value callee, uint8_t argCount)
{
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
                vm.stackTop[- argCount - 1] = bound->receiver;
                return call(bound->method, argCount);
            }
            case OBJ_CLOSURE: {
                return call(AS_CLOSURE(callee), argCount);
            }
            case OBJ_NATIVE: {
                ObjNative* native = AS_NATIVE(callee);
                if (native->arity != argCount) {
                    runtime_error("Expected %d arguments but got %d", native->arity, argCount);
                    return false;
                }

                if (native->function(argCount, vm.stackTop - argCount)) {
                    vm.stackTop -= (uint64_t)argCount;
                    return true;
                } else {
                    runtime_error(AS_CSTRING(vm.stackTop[-argCount]));
                    return false;
                }
            }
            case OBJ_CLASS: {
                ObjClass* loxClass = AS_CLASS(callee);
                vm.stackTop[- argCount - 1] = OBJ_VAL(new_instance(loxClass));

                Value initializer;
                if (table_get(&loxClass->methods, vm.initString, &initializer)) {
                    return call(AS_CLOSURE(initializer), argCount);
                } else if (argCount != 0) {
                    runtime_error("Expected 0 arguments but got %d.", argCount);
                    return false;
                }
                return true;
            }
        }
    }

    runtime_error("Can only call functions and classes.");
    return false;
}

static bool invoke_from_class(ObjClass* loxClass, ObjString* name, uint8_t argCount)
{
    Value method;
    if (!table_get(&loxClass->methods, name, &method)) {
        runtime_error("Undefined property '%s'", name->chars);
        return false;
    }

    return call(AS_CLOSURE(method), argCount);
}

static bool invoke(ObjString* name, uint8_t argCount)
{
    Value receiver = peek(argCount);

    if (!IS_INSTANCE(receiver)) {
        runtime_error("Can only invoke methods on class instances.");
        return false;
    }

    ObjInstance* instance = AS_INSTANCE(receiver);

    Value value;
    if (table_get(&instance->fields, name, &value)) {
        vm.stackTop[- argCount - 1] = value;
        return call_value(value, argCount);
    }

    return invoke_from_class(instance->loxClass, name, argCount);
}

static ObjUpvalue* capture_upvalue(Value* local)
{
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = vm.openUpvalues;

    while (upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue* createdUpvalue = new_upvalue(local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == NULL) {
        vm.openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void close_upvalues(Value* last)
{
    while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
        ObjUpvalue* upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.openUpvalues = upvalue->next;
    }
}

static void define_method(ObjString* name)
{
    Value method = peek(0);
    ObjClass* loxClass = AS_CLASS(peek(1));
    table_put(&loxClass->methods, name, method);
    vm_pop();
}

static bool bind_method(ObjClass* loxClass, ObjString* name)
{
    Value method;
    if (!table_get(&loxClass->methods, name, &method)) {
        runtime_error("Undefined property '%s'.", name->chars);
        return false;
    }

    ObjBoundMethod* bound = new_bound_method(peek(0), AS_CLOSURE(method));
    vm_pop();
    vm_push(OBJ_VAL(bound));
    return true;
}

static void concatenate()
{
    ObjString* b = AS_STRING(peek(0));
    ObjString* a = AS_STRING(peek(1));

    size_t length = a->length + b->length;
    ObjString* string = make_string(length);

    memcpy(string->chars, a->chars, a->length);
    memcpy(string->chars + a->length, b->chars, b->length);
    string->chars[length] = '\0';
    string->hash = hash_string(string->chars, length);

    ObjString* interned = table_find_string(&vm.strings, string->chars, length, string->hash);
    if (interned != NULL) {
        vm.objects = vm.objects->next;
        reallocate(string, sizeof(ObjString) + string->length + 1, 0);

        vm_pop();
        vm_pop();

        vm_push(OBJ_VAL(interned));
    } else {
        vm_push(OBJ_VAL(string));
        table_put(&vm.strings, string, NIL_VAL());
        vm_pop();

        vm_pop();
        vm_pop();

        vm_push(OBJ_VAL(string));
    }
}

static InterpretStatus run()
{
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    register uint8_t* ip = frame->ip;

#define READ_BYTE() (*ip++)
#define READ_SHORT() (ip += 2, (uint16_t)(ip[-2] << 0 | ip[-1] << 8))
#define READ_CONSTANT() frame->closure->function->chunk.constants.values[READ_BYTE()]
#define READ_STRING() AS_STRING(READ_CONSTANT())

    while (true) {
#if DEBUG_TRACE_EXECUTION
        printf("\t");
        for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
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
                vm_push(READ_CONSTANT());
                break;
            }
            case OP_TRUE: {
                vm_push(BOOL_VAL(true));
                break;
            }
            case OP_FALSE: {
                vm_push(BOOL_VAL(false));
                break;
            }
            case OP_NIL: {
                vm_push(NIL_VAL());
                break;
            }
            case OP_NOT_EQUAL: {
                Value b = vm_pop();
                Value a = vm_pop();
                vm_push(BOOL_VAL(!values_equal(a, b)));
                break;
            }
            case OP_EQUAL: {
                Value b = vm_pop();
                Value a = vm_pop();
                vm_push(BOOL_VAL(values_equal(a, b)));
                break;
            }
            case OP_GREATER: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    return runtime_error("Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop());
                double a = AS_NUMBER(vm_pop());
                vm_push(BOOL_VAL(a > b));
                break;
            }
            case OP_GREATER_EQUAL: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    return runtime_error("Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop());
                double a = AS_NUMBER(vm_pop());
                vm_push(BOOL_VAL(a >= b));
                break;
            }
            case OP_LESS: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    return runtime_error("Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop());
                double a = AS_NUMBER(vm_pop());
                vm_push(BOOL_VAL(a < b));
                break;
            }
            case OP_LESS_EQUAL: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    return runtime_error("Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop());
                double a = AS_NUMBER(vm_pop());
                vm_push(BOOL_VAL(a <= b));
                break;
            }
            case OP_NOT: {
                vm.stackTop[-1] = BOOL_VAL(is_falsey(vm.stackTop[-1]));
                break;
            }
            case OP_NEGATE: {
                if (!IS_NUMBER(peek(0))) {
                    return runtime_error("Operand must be a number.");
                }

                vm.stackTop[-1] = NUMBER_VAL(-AS_NUMBER(vm.stackTop[-1]));
                break;
            }
            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) || IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(vm_pop());
                    double a = AS_NUMBER(vm_pop());
                    vm_push(NUMBER_VAL(a + b));
                } else {
                    return runtime_error("Operands must be either numbers or strings.");
                }
                break;
            }
            case OP_SUBTRACT: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    return runtime_error("Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop());
                double a = AS_NUMBER(vm_pop());
                vm_push(NUMBER_VAL(a - b));
                break;
            }
            case OP_MULTIPLY: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    return runtime_error("Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop());
                double a = AS_NUMBER(vm_pop());
                vm_push(NUMBER_VAL(a * b));
                break;
            }
            case OP_DIVIDE: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    return runtime_error("Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop());
                double a = AS_NUMBER(vm_pop());
                vm_push(NUMBER_VAL(a / b));
                break;
            }
            case OP_MODULO: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    return runtime_error("Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop());
                double a = AS_NUMBER(vm_pop());
                vm_push(NUMBER_VAL(fmod(a, b)));
                break;
            }
            case OP_BITWISE_NOT: {
                if (!IS_NUMBER(peek(0))) {
                    return runtime_error("Operand must be a number.");
                }

                vm.stackTop[-1] = NUMBER_VAL((double)(~(int64_t)AS_NUMBER(vm.stackTop[-1])));
                break;
            }
            case OP_BITWISE_AND: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    return runtime_error("Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop());
                double a = AS_NUMBER(vm_pop());
                vm_push(NUMBER_VAL((double)((int64_t)a & (int64_t)b)));
                break;
            }
            case OP_BITWISE_OR: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    return runtime_error("Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop());
                double a = AS_NUMBER(vm_pop());
                vm_push(NUMBER_VAL((double)((int64_t)a | (int64_t)b)));
                break;
            }
            case OP_BITWISE_XOR: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    return runtime_error("Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop());
                double a = AS_NUMBER(vm_pop());
                vm_push(NUMBER_VAL((double)((int64_t)a ^ (int64_t)b)));
                break;
            }
            case OP_BITWISE_LEFT_SHIFT: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    return runtime_error("Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop());
                double a = AS_NUMBER(vm_pop());
                vm_push(NUMBER_VAL((double)((int64_t)a << (int64_t)b)));
                break;
            }
            case OP_BITWISE_RIGHT_SHIFT: {
                if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
                    return runtime_error("Operands must be numbers");
                }

                double b = AS_NUMBER(vm_pop());
                double a = AS_NUMBER(vm_pop());
                vm_push(NUMBER_VAL((double)((int64_t)a >> (int64_t)b)));
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
                if (is_falsey(peek(0))) {
                    ip += offset;
                }
                break;
            }
            case OP_JUMP_IF_NOT_EQUAL: {
                uint16_t offset = READ_SHORT();
                if (!values_equal(peek(0), peek(1))) {
                    ip += offset;
                }
                break;
            }
            case OP_POP: {
                vm_pop();
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString* identifier = READ_STRING();
                table_put(&vm.globals, identifier, peek(0));
                vm_pop();
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* identifier = READ_STRING();
                if (table_put(&vm.globals, identifier, peek(0))) {
                    frame->ip = ip;
                    table_remove(&vm.globals, identifier);
                    return runtime_error("Undefined variable '%s'.", identifier->chars);
                }
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* identifier = READ_STRING();
                Value value;
                if (!table_get(&vm.globals, identifier, &value)) {
                    frame->ip = ip;
                    return runtime_error("Undefined variable '%s'.", identifier->chars);
                }
                vm_push(value);
                break;
            }
            case OP_SET_LOCAL: {
                frame->slots[READ_BYTE()] = peek(0);
                break;
            }
            case OP_GET_LOCAL: {
                vm_push(frame->slots[READ_BYTE()]);
                break;
            }
            case OP_SET_UPVALUE: {
                *frame->closure->upvalues[READ_BYTE()]->location = peek(0);
                break;
            }
            case OP_GET_UPVALUE: {
                vm_push(*frame->closure->upvalues[READ_BYTE()]->location);
                break;
            }
            case OP_SET_PROPERTY: {
                if (!IS_INSTANCE(peek(1))) {
                    runtime_error("Can only set properties of class instances.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = AS_INSTANCE(peek(1));
                table_put(&instance->fields, READ_STRING(), peek(0));

                Value value = vm_pop();
                vm_pop();
                vm_push(value);
                break;
            }
            case OP_GET_PROPERTY: {
                if (!IS_INSTANCE(peek(0))) {
                    runtime_error("Can only access properties of class instances.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = AS_INSTANCE(peek(0));
                ObjString* name = READ_STRING();

                Value value;
                if (table_get(&instance->fields, name, &value)) {
                    vm_pop();
                    vm_push(value);
                    break;
                }

                if (!bind_method(instance->loxClass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                break;
            }
            case OP_PRINT: {
                print_value(vm_pop());
                printf("\n");
                break;
            }
            case OP_CLOSURE: {
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure* closure = new_closure(function);
                vm_push(OBJ_VAL(closure));
                for (size_t i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues[i] = capture_upvalue(frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case OP_CLOSE_UPVALUE: {
                close_upvalues(vm.stackTop - 1);
                vm_pop();
                break;
            }
            case OP_CALL: {
                uint8_t argCount = READ_BYTE();
                frame->ip = ip;

                if (!call_value(peek(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm.frames[vm.frameCount - 1];
                ip = frame->ip;
                break;
            }
            case OP_INVOKE: {
                ObjString* method = READ_STRING();
                uint8_t argCount = READ_BYTE();
                frame->ip = ip;

                if (!invoke(method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm.frames[vm.frameCount - 1];
                ip = frame->ip;
                break;
            }
            case OP_RETURN: {
                Value result = vm_pop();

                close_upvalues(frame->slots);

                vm.frameCount--;
                if (vm.frameCount == 0) {
                    vm_pop();
                    return INTERPRET_OK;
                }

                vm.stackTop = frame->slots;
                vm_push(result);

                frame = &vm.frames[vm.frameCount - 1];
                ip = frame->ip;
                break;
            }
            case OP_CLASS: {
                vm_push(OBJ_VAL(new_class(READ_STRING())));
                break;
            }
            case OP_METHOD: {
                define_method(READ_STRING());
                break;
            }
            case OP_INHERIT: {
                Value superclass = peek(1);
                if (!IS_CLASS(superclass)) {
                    return runtime_error("Superclass must be a class.");
                }

                ObjClass* subclass = AS_CLASS(peek(0));
                table_put_from(&AS_CLASS(superclass)->methods, &subclass->methods);
                vm_pop();
                break;
            }
            case OP_GET_SUPER: {
                ObjString* name = READ_STRING();
                ObjClass* superclass = AS_CLASS(vm_pop());
                if (!bind_method(superclass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUPER_INVOKE: {
                ObjString* method = READ_STRING();
                uint8_t argCount = READ_BYTE();
                ObjClass* superclass = AS_CLASS(vm_pop());
                frame->ip = ip;

                if (!invoke_from_class(superclass, method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm.frames[vm.frameCount - 1];
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

InterpretStatus vm_interpret(const char* source)
{
    ObjFunction* function = compile(source);
    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }

    vm_push(OBJ_VAL(function));
    ObjClosure* closure = new_closure(function);
    vm_pop();
    vm_push(OBJ_VAL(closure));
    call_value(OBJ_VAL(closure), 0);

    return run();
}
