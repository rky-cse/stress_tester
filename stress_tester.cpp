#include <iostream>
#include <chrono>
#include <thread>
#include <future>
#include <atomic>
#include <cstdlib> // For system(), kill()
#include <csignal> // For signal handling
#include <cstdio>  // For popen(), pclose()

#include <string>
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <fstream>
#include <sstream>

// init()========================================================================
const int TIME_LIMIT = 5; // in seconds

const int NUMBER_OF_TEST_CASES = 100;

std::string test_case_generator = "test_case_generator"; // file name without ".cpp";

std::string brute_force_code = "a"; // file name without ".cpp";

std::string optimized_code = "b"; // file name without ".cpp";

//========================================================================

void createFile(const std::string &fileName)
{
    std::ofstream newFile(fileName);
    // if (newFile)
    //  newFile << "This is a newly created file.\n";
}

bool KillProcessByName(const char *filename)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to create snapshot of running processes." << std::endl;
        return false;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hSnapshot, &pe))
    {
        do
        {
            // std::cout << "Checking process: " << pe.szExeFile << std::endl;
            if (strcmp(pe.szExeFile, filename) == 0)
            {
                // std::cout << "Found process: " << pe.szExeFile << " with PID: " << pe.th32ProcessID << std::endl;
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, pe.th32ProcessID);
                if (hProcess != nullptr)
                {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                    // std::cout << "Process " << filename << " terminated successfully." << std::endl;
                    CloseHandle(hSnapshot);
                    return true;
                }
                else
                {
                    std::cerr << "Failed to open process for termination." << std::endl;
                }
            }
        } while (Process32Next(hSnapshot, &pe));
    }

    std::cerr << "Process " << filename << " not found." << std::endl;
    CloseHandle(hSnapshot);
    return false;
}

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>   // For usleep, fork, exec, kill, etc.
#include <sys/wait.h> // For waitpid()
#endif

#ifdef _WIN32
#define COMPILE_COMMAND "g++"
#else
#define COMPILE_COMMAND "g++"
#endif

// Function to read the entire content of a file as a string
std::string read_file(const std::string &filename)
{
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf(); // Read the file into the buffer
    return buffer.str();
}

bool compare(const std::string &s1, const std::string &s2)
{
    std::string result1, result2;
    auto normalize = [](const std::string &s) -> std::string
    {
        std::string res;
        bool in_space = false;
        for (char c : s)
        {
            if (c == ' ' || c == '\n')
            {
                if (!in_space)
                    res += ' ';
                in_space = true;
            }
            else
            {
                res += c;
                in_space = false;
            }
        }
        if (!res.empty() && res.back() == ' ')
            res.pop_back();
        return res;
    };
    return normalize(s1) == normalize(s2);
}

// Get unique identifier based on timestamp
std::string getUniqueIdentifier()
{
    // auto now = std::chrono::system_clock::now();
    // auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    // return std::to_string(milliseconds);
    return "_for_making_process_name_unique";
}

void deleteFile(const char *filename)
{
    if (remove(filename) != 0)
    {
        std::perror("Error deleting file");
    }
    else
    {
        // std::cout << "File deleted successfully" << std::endl;
    }
}

// Function to check if the C++ file exists
bool fileExists(const std::string &filename)
{
    std::string fullFilename = filename + ".cpp"; // Ensure the extension is added
    std::ifstream infile(fullFilename);
    return infile.good();
}

// Function to compile the C++ file with ONLINE_JUDGE macro defined
bool compile(const std::string &filename, const std::string &uniqueID)
{
    if (!fileExists(filename))
    {
        std::cerr << "Error: File not found: " << filename << std::endl;
        return 0;
    }
    std::string unique_name = filename + uniqueID;
    std::string compileCommand = std::string(COMPILE_COMMAND) + " " + filename + ".cpp -o " + unique_name + " -DONLINE_JUDGE";
    // std::cout << "Running compile command: " << compileCommand << std::endl;
    int result = system(compileCommand.c_str());
    return result == 0; // Compilation successful if result is 0
}

void runProgram(const std::string &filename, const std::string &uniqueID, const std::string &input_file_name, const std::string &output_file_name, bool &TLE_FLAG)
{
    TLE_FLAG = false;
    std::string command = filename + uniqueID + "< " + input_file_name + " > " + output_file_name; // Replace "your_program"

    auto start = std::chrono::high_resolution_clock::now();
    std::atomic<bool> finished(false);
    std::condition_variable cv;
    std::mutex mtx;

    std::thread runner([&]()
                       {
                           int result = system(command.c_str());
                           {
                               std::lock_guard<std::mutex> lock(mtx);
                               finished = (result == 0);
                           }
                           cv.notify_one(); // Notify the main thread that the command has finished
                       });

    // Wait for the execution to complete or timeout after 5 seconds
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (!cv.wait_for(lock, std::chrono::seconds(TIME_LIMIT), [&]
                         { return finished.load(); }))
        {
            // If we timed out
            KillProcessByName((filename + uniqueID + ".exe").c_str()); // Replace with the actual process name
            std::cerr << "Time Taken by " << filename << ": TIME LIMIT EXCEEDED\n";
            TLE_FLAG = true;
        }
    }

    // Check if the command completed successfully before timeout
    if (finished)
    {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        // std::cout << "Program executed successfully." << std::endl;
        std::cerr << "Time Taken By " << filename << ": " << (elapsed.count()) * 1000 << " ms.\n";
    }
    // Ensure thread cleanup
    runner.join();
}

int main()
{

    createFile("test_case_generator_input.txt");
    createFile("stress_tester_input_test_case.txt");
    createFile("brute_force_code_output.txt");
    createFile("optimized_code_output.txt");

    // Step 2: Compile the brute force code with ONLINE_JUDGE defined
    std::string test_case_generator_uniqueID = getUniqueIdentifier();
    std::cerr << "Compiling " << test_case_generator << ".cpp ...\n\n";
    if (!compile(test_case_generator, test_case_generator_uniqueID))
    {
        std::cerr << "Error: Compilation failed!" << std::endl;
        return 1;
    }
    std::cout << test_case_generator << ".cpp Compilation successful!\n\n";

    // Step 2: Compile the brute force code with ONLINE_JUDGE defined
    std::string brute_force_code_uniqueID = getUniqueIdentifier();
    std::cout << "Compiling " << brute_force_code << ".cpp ...\n\n";
    if (!compile(brute_force_code, brute_force_code_uniqueID))
    {
        std::cerr << "Error: Compilation failed!" << std::endl;
        return 1;
    }
    std::cout << brute_force_code << ".cpp Compilation successful!\n\n";

    // Step 2: Compile the optimized code with ONLINE_JUDGE defined
    std::string optimized_code_uniqueID = getUniqueIdentifier();
    std::cerr << "Compiling " << optimized_code << ".cpp ...\n\n";
    if (!compile(optimized_code, optimized_code_uniqueID))
    {
        std::cerr << "Error: Compilation failed!" << std::endl;
        return 1;
    }
    std::cerr << optimized_code << ".cpp Compilation successful!\n\n";

    for (int i = 1; i <= NUMBER_OF_TEST_CASES; i++)
    {
        bool TLE_FLAG = false;
        bool BRUTE_TLE_FLAG = false;

        runProgram(test_case_generator, test_case_generator_uniqueID, "test_case_generator_input.txt", "stress_tester_input_test_case.txt", TLE_FLAG);
        if (TLE_FLAG)
        {
            std::cerr << "\ntest_case_generator crashed\n\n";
            continue;
        }
        runProgram(brute_force_code, brute_force_code_uniqueID, "stress_tester_input_test_case.txt", "brute_force_code_output.txt", BRUTE_TLE_FLAG);
        runProgram(optimized_code, optimized_code_uniqueID, "stress_tester_input_test_case.txt", "optimized_code_output.txt", TLE_FLAG);
        if (!BRUTE_TLE_FLAG and TLE_FLAG)
        {
            std::cerr << "\nFailed on Test Case " << i << "\n\n\n";
            std::cerr << "input: \n"
                      << read_file("stress_tester_input_test_case.txt") << "\n\n";
            std::cerr << "brute_force_code output:\n"
                      << read_file("brute_force_code_output.txt") << "\n\n";

            break;
        }
        else if (BRUTE_TLE_FLAG and TLE_FLAG)
        {
            std::cerr << "\nFailed on Test Case " << i << "\n\n\n";
            std::cerr << "input: \n"
                      << read_file("stress_tester_input_test_case.txt") << "\n\n";
            break;
        }
        else if (BRUTE_TLE_FLAG and !TLE_FLAG)
        {
            std::cerr << "\noptimized code ran on testcase: " << i << "\n\n";
            std::cerr << "input: \n"
                      << read_file("stress_tester_input_test_case.txt") << "\n\n";
            std::cerr << "optimized_code_output: \n"
                      << read_file("optimized_code_output.txt") << "\n\n";
            continue;
        }

        std::string b = read_file(("brute_force_code_output.txt"));
        std::string o = read_file(("optimized_code_output.txt"));

        if (compare(b, o))
        {
            std::cerr << "\nTest Case " << i << " Passed\n\n\n";
        }
        else
        {
            std::cerr << "\nFailed on Test Case " << i << "\n\n\n";
            std::cerr << "input: \n"
                      << read_file("stress_tester_input_test_case.txt") << "\n\n";
            std::cerr << "optimized_code_output: \n"
                      << o << "\n\n";
            std::cerr << "brute_force_code_output: \n"
                      << b << "\n\n";

            break;
        }
    }

    deleteFile((test_case_generator + test_case_generator_uniqueID + ".exe").c_str());
    deleteFile((brute_force_code + brute_force_code_uniqueID + ".exe").c_str());
    deleteFile((optimized_code + optimized_code_uniqueID + ".exe").c_str());

    return 0;
}
