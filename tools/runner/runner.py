import sys
import os
import subprocess
import errno

class TestResult:
    def __init__(self, successful_count, total, file):
        self.successful_count = successful_count
        self.total = total
        self.file = file
        
def main():
    if len(sys.argv) == 1:
        help([])
        exit(0)
    
    dispatch = {
        "help": help,
        "run": run
    }
    
    command = sys.argv[1]
    args = sys.argv[2:]
    
    if command not in dispatch:
        exit("Unknown command: '%s'" % command)
    
    dispatch[command](args)

def help(args):
    print("Usage: runner.py <command> <args...>")
    print("Commands: ")
    print("- help: prints usage information.")
    print("- run <program> <test>: runs specified 'test' against 'program'.")
    print("\tIf 'test' is a single file, it will be run directly.")
    print("\tIf 'test' is a directory, the runner will walk it recursively and execute each encountered test file.")

def run(args):
    if len(args) < 2:
        print("Not enough arguments to run. Refer to 'help' for more information.")
        return
    
    program = args[0]
    path = args[1]
    
    if os.path.isfile(path):
        run_file(program, path)
    elif os.path.isdir(path):
        run_directory(program, path)
    else:
        print("Specified path is not recognized as an existing file or directory: '%s'" % path)
        
def run_file(program, file):
    if not is_valid_file(file):
        print("Can only run files with '.lox' extension.")
        return
    
    result = run_test(program, file)
    if result != None:
        print("Successfully completed %d out of %d tests." % (result.successful_count, result.total))

def run_directory(program, directory):
    successful_count = 0
    total = 0
    
    for root, _, files in os.walk(directory):
        for file in files:
            if not is_valid_file(file):
                continue
            
            complete_path = os.path.join(root, file)
            result = run_test(program, complete_path)
            
            if result == None:
                return
            
            successful_count += result.successful_count
            total += result.total
            
    if total == 0:
        print("Specified directory does not contain any valid tests.")
    else:
        print("Successfully completed %d out of %d tests." % (successful_count, total))
        
def run_test(program, file):
    print("Running tests in file '%s'..." % file)
    
    expected = parse_expected(file)
    expected_count = len(expected)
        
    output = None
    try:
        output = subprocess.check_output([program, file])
    except OSError:
        print("Couldn't run program at '%s'." % program)
        return
    except subprocess.CalledProcessError as err:
        print("File '%s', containing %d tests, could not be run." % (file, expected_count))
        print("The command used to run the process: %s" % err.cmd)
        print("The output of the process: %s" % err.output)
        print("The process finished running with code %d." % err.returncode)
        return TestResult(0, expected_count, file)
    
    actual = list(map(lambda x: x.decode(), output.splitlines()))
    actual_count = len(actual)

    if expected_count != actual_count:
        print("Expected %d results but got %d in %s." % (expected_count, actual_count, file))
        print("- Expected: %s" % str(expected))
        print("- Actual: %s" % str(actual))
        return TestResult(0, expected_count, file)
    
    successful_count = 0
    for expected_item, actual_item in zip(expected, actual):
        if expected_item != actual_item:
            print("Result mismatch: expected '%s' but got '%s' in '%s'." % (expected_item, actual_item, file))
        else:
            successful_count += 1
            
    if (successful_count == actual_count):
       print("File '%s', containing %d tests, has finished running without errors." % (file, actual_count))
            
    return TestResult(successful_count, actual_count, file)

def is_valid_file(file):
    return file.rpartition('.')[-1].lower() == "lox"

def parse_expected(file):
    expected = []
    
    reader = open(file, "r")
    for line in reader:
        (_, _, comment) = line.partition("//")
        
        comment = comment.strip()        
        if comment.startswith("Expected:"):
            (_, _, value) = comment.partition(':')
            expected.append(value.strip())
            
    return expected

if __name__ == "__main__":
    main()