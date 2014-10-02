/*  Copyright 2014 Aaron Boxer (boxerab@gmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include "OCLBasic.h"

#include <iostream>
#include <exception>
#include <vector>
#include <cerrno>

#ifdef __linux__
#include <sys/time.h>
#include <unistd.h>
#include <libgen.h>
#elif defined(_WIN32) || defined(WIN32)
#include <windows.h>
#else
#include <ctime>
#endif

using std::string;


string opencl_error_to_str (cl_int error)
{
#define CASE_CL_CONSTANT(NAME) case NAME: return #NAME;

    // Suppose that no combinations are possible.
    // TODO: Test whether all error codes are listed here
    switch(error)
    {
        CASE_CL_CONSTANT(CL_SUCCESS)
        CASE_CL_CONSTANT(CL_DEVICE_NOT_FOUND)
        CASE_CL_CONSTANT(CL_DEVICE_NOT_AVAILABLE)
        CASE_CL_CONSTANT(CL_COMPILER_NOT_AVAILABLE)
        CASE_CL_CONSTANT(CL_MEM_OBJECT_ALLOCATION_FAILURE)
        CASE_CL_CONSTANT(CL_OUT_OF_RESOURCES)
        CASE_CL_CONSTANT(CL_OUT_OF_HOST_MEMORY)
        CASE_CL_CONSTANT(CL_PROFILING_INFO_NOT_AVAILABLE)
        CASE_CL_CONSTANT(CL_MEM_COPY_OVERLAP)
        CASE_CL_CONSTANT(CL_IMAGE_FORMAT_MISMATCH)
        CASE_CL_CONSTANT(CL_IMAGE_FORMAT_NOT_SUPPORTED)
        CASE_CL_CONSTANT(CL_BUILD_PROGRAM_FAILURE)
        CASE_CL_CONSTANT(CL_MAP_FAILURE)
        CASE_CL_CONSTANT(CL_MISALIGNED_SUB_BUFFER_OFFSET)
        CASE_CL_CONSTANT(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST)
        CASE_CL_CONSTANT(CL_INVALID_VALUE)
        CASE_CL_CONSTANT(CL_INVALID_DEVICE_TYPE)
        CASE_CL_CONSTANT(CL_INVALID_PLATFORM)
        CASE_CL_CONSTANT(CL_INVALID_DEVICE)
        CASE_CL_CONSTANT(CL_INVALID_CONTEXT)
        CASE_CL_CONSTANT(CL_INVALID_QUEUE_PROPERTIES)
        CASE_CL_CONSTANT(CL_INVALID_COMMAND_QUEUE)
        CASE_CL_CONSTANT(CL_INVALID_HOST_PTR)
        CASE_CL_CONSTANT(CL_INVALID_MEM_OBJECT)
        CASE_CL_CONSTANT(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR)
        CASE_CL_CONSTANT(CL_INVALID_IMAGE_SIZE)
        CASE_CL_CONSTANT(CL_INVALID_SAMPLER)
        CASE_CL_CONSTANT(CL_INVALID_BINARY)
        CASE_CL_CONSTANT(CL_INVALID_BUILD_OPTIONS)
        CASE_CL_CONSTANT(CL_INVALID_PROGRAM)
        CASE_CL_CONSTANT(CL_INVALID_PROGRAM_EXECUTABLE)
        CASE_CL_CONSTANT(CL_INVALID_KERNEL_NAME)
        CASE_CL_CONSTANT(CL_INVALID_KERNEL_DEFINITION)
        CASE_CL_CONSTANT(CL_INVALID_KERNEL)
        CASE_CL_CONSTANT(CL_INVALID_ARG_INDEX)
        CASE_CL_CONSTANT(CL_INVALID_ARG_VALUE)
        CASE_CL_CONSTANT(CL_INVALID_ARG_SIZE)
        CASE_CL_CONSTANT(CL_INVALID_KERNEL_ARGS)
        CASE_CL_CONSTANT(CL_INVALID_WORK_DIMENSION)
        CASE_CL_CONSTANT(CL_INVALID_WORK_GROUP_SIZE)
        CASE_CL_CONSTANT(CL_INVALID_WORK_ITEM_SIZE)
        CASE_CL_CONSTANT(CL_INVALID_GLOBAL_OFFSET)
        CASE_CL_CONSTANT(CL_INVALID_EVENT_WAIT_LIST)
        CASE_CL_CONSTANT(CL_INVALID_EVENT)
        CASE_CL_CONSTANT(CL_INVALID_OPERATION)
        CASE_CL_CONSTANT(CL_INVALID_GL_OBJECT)
        CASE_CL_CONSTANT(CL_INVALID_BUFFER_SIZE)
        CASE_CL_CONSTANT(CL_INVALID_MIP_LEVEL)
        CASE_CL_CONSTANT(CL_INVALID_GLOBAL_WORK_SIZE)
        CASE_CL_CONSTANT(CL_INVALID_PROPERTY)

    default:
        return "UNKNOWN ERROR CODE " + to_str(error);
    }

#undef CASE_CL_CONSTANT
}


double  my_clock(void) {

#ifdef _WIN32
	/* _WIN32: use QueryPerformance (very accurate) */
    LARGE_INTEGER freq , t ;
    /* freq is the clock speed of the CPU */
    QueryPerformanceFrequency(&freq) ;
	/* cout << "freq = " << ((double) freq.QuadPart) << endl; */
    /* t is the high resolution performance counter (see MSDN) */
    QueryPerformanceCounter ( & t ) ;
    return ( t.QuadPart /(double) freq.QuadPart ) ;
#endif
#ifdef __linux__
	return (double)(t.tv_sec + t.tv_usec/1e6); 
#endif
}

void* aligned_malloc (size_t size, size_t alignment)
{
    // a number of requirements should be met
    assert(alignment > 0);
    assert((alignment & (alignment - 1)) == 0); // test for power of 2

    if(alignment < sizeof(void*))
    {
        alignment = sizeof(void*);
    }

    assert(size >= sizeof(void*));

    //why is this necessary??
    //assert(size/sizeof(void*)*sizeof(void*) == size);

    // allocate extra memory and convert to size_t to perform calculations
    char* orig = new char[size + alignment + sizeof(void*)];
    // calculate an aligned position in the allocated region
    // assumption: (size_t)orig does not lose lower bits
    char* aligned =
        orig + (
        (((size_t)orig + alignment + sizeof(void*)) & ~(alignment - 1)) -
        (size_t)orig
        );
    // save the original pointer to use it in aligned_free
    *((char**)aligned - 1) = orig;
    return aligned;
}


void aligned_free (void *aligned)
{
    if(!aligned)return; // behaves as delete: calling with 0 is NOP
    delete [] *((char**)aligned - 1);
}


bool is_number (const string& x)
{
    // Detection is simple: just try to represent x as an int
    try
    {
        // If x is a number, then str_to returns without an exception
        // In case when x cannot be converted to int
        // str_to rises Error exception (see str_to definitin)
        str_to<int>(x);

        // success: x is a number
        return true;
    }
    catch(const Error&)
    {
        // fail: x is not a number
        return false;
    }
}


double time_stamp ()
{
#ifdef __linux__
    {
        struct timeval t;
        if(gettimeofday(&t, 0) != 0)
        {
            throw Error(
                "Linux-specific time measurement counter (gettimeofday) "
                "is not available."
                );
        }
        return t.tv_sec + t.tv_usec/1e6;
    }
#elif defined(_WIN32) || defined(WIN32)
    {
        LARGE_INTEGER curclock;
        LARGE_INTEGER freq;
        if(
            !QueryPerformanceCounter(&curclock) ||
            !QueryPerformanceFrequency(&freq)
            )
        {
            throw Error(
                "Windows-specific time measurement counter (QueryPerformanceCounter, "
                "QueryPerformanceFrequency) is not available."
                );
        }

        return double(curclock.QuadPart)/freq.QuadPart;
    }
#else
    {
        // very low resolution
        return double(time(0));
    }
#endif
}


void destructorException ()
{
    if(std::uncaught_exception())
    {
        // don't crash an application because of double throwing
        // let the user see the original exception and suppress
        // this one instead
        std::cerr
            << "[ ERROR ] Catastrophic: another exception "
            << "was thrown and suppressed during handling of "
            << "previously started exception raising process.\n";
    }
    else
    {
        // that's OK, go up!
        throw;
    }
}


cl_uint requiredOpenCLAlignment (cl_device_id device)
{
    cl_uint result = 0;
    cl_int err = clGetDeviceInfo(
        device,
        CL_DEVICE_MEM_BASE_ADDR_ALIGN,
        sizeof(result),
        &result,
        0
        );
    SAMPLE_CHECK_ERRORS(err);
    assert(result%8 == 0);
    return result/8;    // clGetDeviceInfo returns value in bits, convert it to bytes
}


size_t deviceMaxWorkGroupSize (cl_device_id device)
{
    size_t result = 0;
    cl_int err = clGetDeviceInfo(
        device,
        CL_DEVICE_MAX_WORK_GROUP_SIZE,
        sizeof(result),
        &result,
        0
        );
    SAMPLE_CHECK_ERRORS(err);
    return result;
}


void deviceMaxWorkItemSizes (cl_device_id device, size_t* sizes)
{
    cl_int err = clGetDeviceInfo(
        device,
        CL_DEVICE_MAX_WORK_ITEM_SIZES,
        sizeof(size_t[3]),
        sizes,
        0
        );
    SAMPLE_CHECK_ERRORS(err);
}


size_t kernelMaxWorkGroupSize (cl_kernel kernel, cl_device_id device)
{
    size_t result = 0;
    cl_int err = clGetKernelWorkGroupInfo(
        kernel,
        device,
        CL_KERNEL_WORK_GROUP_SIZE,
        sizeof(result),
        &result,
        0
        );
    SAMPLE_CHECK_ERRORS(err);
    return result;
}


double eventExecutionTime (cl_event event)
{
    cl_ulong end = 0, start = 0;

    cl_int err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(start), &start, 0);
    SAMPLE_CHECK_ERRORS(err);

    err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(end), &end, 0);
    SAMPLE_CHECK_ERRORS(err);

    return double(end - start)/1e9; // convert in seconds
}


string exe_dir ()
{
    using namespace std;
    const int start_size = 1000;
    const int max_try_count = 8;

#ifdef __linux__
    {
        static string const exe = "/proc/self/exe";

        vector<char> path(start_size);
        int          count = max_try_count;  // Max number of iterations.

        for(;;)
        {
            ssize_t len = readlink(exe.c_str(), &path[0], path.size());

            if(len < 0)
            {
                throw Error(
                    "Cannot retrieve path to the executable: "
                    "readlink returned error code " +
                    to_str(errno) + "."
                    );
            }

            if(len < path.size())
            {
                // We got the path.
                path.resize(len);
                break;
            }

            if(count > 0)   // the buffer is too small
            {
                --count;
                // Enlarge the buffer.
                path.resize(path.size() * 2);
            }
            else
            {
                throw Error("Cannot retrieve path to the executable: path is too long.");
            }
        }

        return string(dirname(&path[0])) + "/";
    }
#elif defined(_WIN32) || defined(WIN32)
    {
        // Retrieving path to the executable.

        vector<char> path(start_size);
        int count = max_try_count;

        for(;;)
        {
            DWORD len = GetModuleFileNameA(NULL, &path[0], (DWORD)path.size());

            if(len == 0)
            {
                int err = GetLastError();
                throw Error(
                    "Getting executable path failed with error " +
                    to_str(err)
                    );
            }

            if(len < path.size())
            {
                path.resize(len);
                break;
            }

            if(count > 0)   // buffer is too small
            {
                --count;
                // Enlarge the buffer.
                path.resize(path.size() * 2);
            }
            else
            {
                throw Error(
                    "Cannot retrieve path to the executable: "
                    "path is too long."
                    );
            }
        }

        string exe(&path[0], path.size());

        // Splitting the path into components.

        vector<char> drv(_MAX_DRIVE);
        vector<char> dir(_MAX_DIR);
        count = max_try_count;

        for(;;)
        {
            int rc =
                _splitpath_s(
                exe.c_str(),
                &drv[0], drv.size(),
                &dir[0], dir.size(),
                NULL, 0,   // We need neither name
                NULL, 0    // nor extension
                );
            if(rc == 0)
            {
                break;
            }
            else if(rc == ERANGE)
            {
                if(count > 0)
                {
                    --count;
                    // Buffer is too small, but it is not clear which one.
                    // So we have to enlarge both.
                    drv.resize(drv.size() * 2);
                    dir.resize(dir.size() * 2);
                }
                else
                {
                    throw Error(
                        "Getting executable path failed: "
                        "Splitting path " + exe + " to components failed: "
                        "Buffers of " + to_str(drv.size()) + " and " +
                        to_str(dir.size()) + " bytes are still too small."
                        );
                }
            }
            else
            {
                throw Error(
                    "Getting executable path failed: "
                    "Splitting path " + exe +
                    " to components failed with code " + to_str(rc)
                    );
            }
        }

        // Combining components back to path.
        return string(&drv[0]) + string(&dir[0]);
    }
#else
    {
        throw Error(
            "There is no method to retrieve the directory path "
            "where executable is placed: unsupported platform."
            );
    }
#endif
}