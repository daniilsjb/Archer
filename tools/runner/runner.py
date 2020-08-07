import sys
import os
import subprocess

class TestResult:
    def __init__(self, successful_count, total_count, interpreter, file):
        self.successful_count = successful_count
        self.total_count = total_count
        self.interpreter = interpreter
        self.file = file
        
class BenchmarkResult:
    def __init__(self, elapsed, interpreter, file):
        self.elapsed = elapsed
        self.interpreter = interpreter
        self.file = file
        
def main():
    if len(sys.argv) == 1:
        help([])
        exit(0)
    
    dispatch = {
        "help": help,
        "test": test,
        "benchmark": benchmark
    }
    
    command = sys.argv[1]
    args = sys.argv[2:]
    
    if command not in dispatch:
        exit("Unknown command: '%s'" % command)
    
    dispatch[command](args)

def help(args):
    print("Usage: runner.py <command> <args...>")
    print("Commands:")
    print("- help: prints usage information.")
    print("- test <interpreter> <test-path>: tests file or directory at 'test-path' against 'interpreter'.")
    print("\tIf 'test-path' is a single file, it will be run directly.")
    print("\tIf 'test-path' is a directory, the runner will walk it recursively and execute each encountered test file.")
    print("- benchmark <interpreter> <benchmark-path>: benchmarks file or directory at 'benchmark-path' against 'interpreter'.")
    print("\tIf 'benchmark-path' is a single file, it will be run directly.")
    print("\tIf 'benchmark-path' is a directory, the runner will walk it recursively and execute each encountered benchmark file.")

def test(args):
    if len(args) < 2:
        exit("Not enough arguments to run. Refer to 'help' for more information.")
    
    interpreter = args[0]
    path = args[1]
    
    if os.path.isfile(path):
        test_file(interpreter, path)
    elif os.path.isdir(path):
        test_directory(interpreter, path)
    else:
        exit("Specified path is not recognized as an existing file or directory: '%s'" % path)
        
def test_file(interpreter, file):
    if not is_valid_file(file):
        exit("Can only run files with '.lox' extension.")
    
    result = run_test(interpreter, file)
    print("Successfully completed %d out of %d tests." % (result.successful_count, result.total_count))

def test_directory(interpreter, directory):
    successful_count = 0
    total_count = 0
    
    for root, _, files in os.walk(directory):
        for file in files:
            if not is_valid_file(file):
                continue
            
            complete_path = os.path.join(root, file)
            result = run_test(interpreter, complete_path)
            
            successful_count += result.successful_count
            total_count += result.total_count
            
    if total_count == 0:
        print("Specified directory does not contain any valid tests.")
    else:
        print("Successfully completed %d out of %d tests." % (successful_count, total_count))
        
def run_test(interpreter, file):
    print("Running tests in file '%s'..." % file)
    
    expected = parse_expected(file)
    expected_count = len(expected)

    actual = get_interpreter_output(interpreter, file)
    if actual == None:
        print("File '%s', containing %d tests, could not be run." % (file, expected_count))
        return TestResult(0, expected_count, interpreter, file)
    
    actual_count = len(actual)

    if expected_count != actual_count:
        print("Expected %d results but got %d in %s." % (expected_count, actual_count, file))
        print("- Expected: %s" % str(expected))
        print("- Actual: %s" % str(actual))
        return TestResult(0, expected_count, interpreter, file)
    
    successful_count = 0
    for expected_item, actual_item in zip(expected, actual):
        if expected_item[0] != actual_item:
            print("Result mismatch: expected '%s' but got '%s' in '%s' on line %d." % (expected_item[0], actual_item, file, expected_item[1]))
        else:
            successful_count += 1
            
    if (successful_count == actual_count):
       print("File '%s', containing %d tests, has finished running without errors." % (file, actual_count))
            
    return TestResult(successful_count, actual_count, interpreter, file)

def parse_expected(file):
    expected = []
    line_count = 1
    
    reader = open(file, "r")
    for line in reader:
        (_, _, comment) = line.partition("//")
        
        comment = comment.strip()        
        if comment.startswith("Expected:"):
            (_, _, value) = comment.partition(':')
            expected.append((value.strip(), line_count))
            
        line_count += 1
            
    return expected

def benchmark(args):
    if (len(args) < 2):
        print("Not enough arguments to run benchmark. Refer to 'help' for more information.")
        return
        
    interpreter = args[0]
    path = args[1]
    
    if os.path.isfile(path):
        benchmark_file(interpreter, path)
    elif os.path.isdir(path):
        benchmark_directory(interpreter, path)
    else:
        exit("Specified path is not recognized as an existing file or directory: '%s'" % path)
        
def benchmark_file(interpreter, file):
    if not is_valid_file(file):
        print("Can only benchmark files with '.lox' extension.")
        return
    
    run_benchmark(interpreter, file)

def benchmark_directory(interpreter, directory):
    for root, _, files in os.walk(directory):
        for file in files:
            if not is_valid_file(file):
                continue
            
            complete_path = os.path.join(root, file)
            run_benchmark(interpreter, complete_path)

def run_benchmark(interpreter, file):
    print("Running benchmark in file '%s'..." % file)
    
    output = get_interpreter_output(interpreter, file)
    if output == None:
        exit("Benchmark '%s' failed to run." % file)
        
    elapsed = output[-1]
    print("Benchmark '%s' run with '%s' took %s seconds to complete." % (file, interpreter, elapsed))
    
    return BenchmarkResult(elapsed, interpreter, file)

def get_interpreter_output(interpreter, file):
    output = None
    try:
        output = subprocess.check_output([interpreter, file])
    except OSError:
        exit("Couldn't run interpreter at '%s'." % interpreter)
    except subprocess.CalledProcessError:
        return
        
    return [x.decode() for x in output.splitlines()]

def is_valid_file(file):
    return file.rpartition('.')[-1].lower() == "lox"

if __name__ == "__main__":
    main()