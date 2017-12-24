# Simple Example

When you don't need to allocate memory or aquire/release resources, the code is simple:

```
#include <c_test/c_test.h>

#include <math.h>

TEST(math, round) {
    int rounded = round(0.6);
    int expected = 1;
    ASSERT_EQ(expected, rounded, "expected %d == %d", rounded, expected);
}

int main(int argc, const char **argv) {
    return RUN_ALL_TESTS();
}
```

You will need to link against `ctest`, which provides functions to run all tests and a default runner that prints
errors to standard output.

# Test Fixture Example

When you need to allocate memory and or aquire some other resource, you should create a text fixture that defines
construction and destruction logic. The same fixture can be used across multiple tests where appropriate:

```
#include <c_test/c_test.h>

typedef struct {
	int *values;
} integer_test_t;

void* integer_test_setup() {
	integer_test_t *test_data = (integer_test_t*)malloc(sizeof(int));
	test_data->values = (int*)malloc(sizeof(int) * 100);
    for (int i = 0; i < 100; i++) {
        test_data->values[i] = i;
    }
	return test_data;
}

void integer_test_teardown(void *data) {
	integer_test_t *test_data = (integer_test_t*)data;
	free(test_data->values);
    free(test_data);
}

c_test_fixture_t IntegerTest = {
	.setup = integer_test_setup,
	.teardown = integer_test_teardown,
};

TEST_F(IntegerTest, ShiftEqualsDivide) {
    integer_test_t *test_data = (integer_test_t*)(data_);
    int *values = test_data->values;
    for (int i = 0; i < 100; i++) {
        EXPECT_EQ(values[i] / 2, values[i] >> 1, "failed for test %d", i);
    }
}

TEST_F(IntegerTest, ShiftEqualsMultiply) {
    integer_test_t *test_data = (integer_test_t*)(data_);
    int *values = test_data->values;
    for (int i = 0; i < 100; i++) {
        EXPECT_EQ(values[i] * 2, values[i] << 1, "failed for test %d", i);
    }
}

int main(int argc, const char **argv) {
    return RUN_ALL_TESTS();
}
```

# Installing

By default the project builds as a shared library, including a printf-style reporter.

```
mkdir -p build
cd build && cmake .. && make
sudo make install
```

However, this printf style reporter may not play nicely with embedded devices. If you

# FAQ

* Why not GoogleTest?
  * GoogleTest is fantastic, but by design requires C++. This only requires C with support for the constructor 
    attribute supported in GCC and Clang. This makes it more appropriate for embedded environments and toolchains 
    you dont have as much flexibility over requiring C+GCC.
* Why not export JSON or XML?
  * I want to keep things simple here and instead expose the runner_t interface allowing users to customize their 
    output. However I would welcome additions here!
*  Why force a format string?
  * This was tough, but it's generally a good practice in unit tests to document the failure cases. In the worst 
    case you can just write `ASSERT_TRUE(<expression>, "");` This seems ideal versus other options like function 
    chaining where it's not clear where the chain ends. For example, having code like 
    `ASSERT_NE(data, NULL)("data failed to initialize")` or 
    `ASSERT_NE(data, NULL)->message("data failed to initialize")` is beautiful in it's own way, but adds overhead to 
    the implementation complexity to buffer around values until either the test finishes executing or the test 
    completes without any significant reduction in boilerplate.  
* This isn't exactly an xUnit clone. You've merged the "runner" and "test result formatter", and don't 
  support test suites.
  * This is correct. I'd be happy to accept MR with these additions but required additional complexity 
    to support and aren't required for my common usecases.
*  This is a lot of macro code. Did you generate it all by hand?
  * No. I generated it with python.
* Why did you internally use `__runner` and not `runner__`?
  * I didn't want to pollute any autocomplete for an argument that typically should not be used.
