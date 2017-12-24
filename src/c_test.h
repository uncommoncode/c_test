#pragma once

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct c_test_runner;

typedef void* (*c_test_setup_function_t)(void);
typedef void (*c_test_teardown_function_t)(void*);

typedef void (*c_test_fixture_function_t)(struct c_test_runner *, void*);
typedef void (*c_test_function_t)(struct c_test_runner *);

typedef struct {
	c_test_setup_function_t setup;
	c_test_teardown_function_t teardown;

	const char *name_space;
	const char *test_name;

	const char *file_name;
	int line_number;

	c_test_fixture_function_t test_fixture_function;
	c_test_function_t test_function;
} c_test_definition_t;

typedef struct {
	c_test_setup_function_t setup;
	c_test_teardown_function_t teardown;
} c_test_fixture_t;


void c_test_add_test_fixture(c_test_fixture_t fixture, c_test_fixture_function_t function, const char *fixture_name, 
                      const char *test_name, const char *file_name, int line_number);
void c_test_add_test(c_test_function_t function, const char *name_space, const char *test_name, const char *file_name, 
              int line_number);

#define ATTRIBUTE_PRINT_FORMAT(format_index, vararg_index) \
	__attribute__ ((format (printf, format_index, vararg_index)))

typedef struct c_test_runner {
	// Called within asserts to pass along error messages.
	void (*failure)(struct c_test_runner *runner, const char *expression, const char *file_name, int line_number, 
					const char *format, ...) ATTRIBUTE_PRINT_FORMAT(5, 6);
	void (*error)(struct c_test_runner *runner, const char *expression, const char *file_name, int line_number, 
				  const char *format, ...) ATTRIBUTE_PRINT_FORMAT(5, 6);
	void (*success)(struct c_test_runner *runner, const char *file_name, int line_number);
	void (*destroy)(struct c_test_runner *runner);

	// Called before and after a test runs.
	void (*begin_test)(struct c_test_runner *runner);
	void (*end_test)(struct c_test_runner *runner, int test_success);
	
	// Called before and after all tests run.
	void (*begin_tests)(struct c_test_runner *runner, uint32_t test_count);
	void (*end_tests)(struct c_test_runner *runner, uint32_t test_count, uint32_t failed_count);

	uint32_t error_count;

	void *data;
	const c_test_definition_t *current_test;
} c_test_runner_t;

c_test_runner_t* c_test_create_default_runner();

int c_test_run(c_test_runner_t* runner);

#ifdef __cplusplus
}
#endif

// === Dont look behind the curtains here ;) === //

#define C_TEST_STR0(x) #x
#define C_TEST_STR(x) C_TEST_STR0(x)

#define C_TEST_CONSTRUCTOR_FUNCTION(_name) \
	__attribute__ ((constructor)) void __c_test_ctor_ ## _name  ()

#define C_TEST_TEST_FIXTURE2(fixture, test_name, fixture_name_cstr, test_name_cstr, file_cstr, line, test_symbol)  \
    /* Forward declare test body */ \
    void test_symbol (c_test_runner_t *, void*); \
	/* Attach test body to our global context */ \
	C_TEST_CONSTRUCTOR_FUNCTION(fixture ## _ ## test_name ## _ ## line) { \
		c_test_add_test_fixture(fixture, test_symbol, fixture_name_cstr, test_name_cstr, file_cstr, line); \
	} \
	/* Declare test function */ \
	void test_symbol (c_test_runner_t *__runner, void* data_)

#define C_TEST_TEST_FIXTURE1(fixture, test_name, fixture_name_cstr, test_name_cstr, file_cstr, line)  \
	C_TEST_TEST_FIXTURE2(fixture, test_name, fixture_name_cstr, test_name_cstr, file_cstr, line, __ctest_test_ ## fixture ## _ ## test_name ## _ ## line )

#define C_TEST_TEST_FIXTURE0(fixture, test_name, fixture_name_cstr, test_name_cstr, file_cstr, line) \
	C_TEST_TEST_FIXTURE1(fixture, test_name, fixture_name_cstr, test_name_cstr, file_cstr, line)

#define C_TEST_TEST2(namespace, test_name, namespace_name_cstr, test_name_cstr, file_cstr, line, test_symbol)  \
    /* Forward declare test body */ \
    void test_symbol (c_test_runner_t *); \
	/* Attach test body to our global context */ \
	C_TEST_CONSTRUCTOR_FUNCTION(namespace ## _ ## test_name ## _ ## line) { \
		c_test_add_test(test_symbol, namespace_name_cstr, test_name_cstr, file_cstr, line); \
	} \
	/* Declare test function */ \
	void test_symbol (c_test_runner_t *__runner)

#define C_TEST_TEST1(namespace, test_name, namespace_name_cstr, test_name_cstr, file_cstr, line)  \
	C_TEST_TEST2(namespace, test_name, namespace_name_cstr, test_name_cstr, file_cstr, line, __c_test_test_ ## namespace ## _ ## test_name ## _ ## line )

#define C_TEST_TEST0(namespace, test_name, namespace_name_cstr, test_name_cstr, file_cstr, line) \
	C_TEST_TEST1(namespace, test_name, namespace_name_cstr, test_name_cstr, file_cstr, line)


// === Assertion Macros === //

#define FAIL(...) return __runner->failure(__runner, "", __FILE__, __LINE__, __VA_ARGS__);
#define SUCCESS(...) return __runner->success(__runner, "", __FILE__, __LINE__, __VA_ARGS__);

#define ASSERT_FALSE(a, ...) if ((a)) { return __runner->failure(__runner,  "ASSERT_FALSE("#a")", __FILE__, __LINE__, __VA_ARGS__); }
#define ASSERT_TRUE(a, ...) if (!((a))) { return __runner->failure(__runner,  "ASSERT_TRUE("#a")", __FILE__, __LINE__, __VA_ARGS__); }

#define ASSERT_GT(a, b, ...) if ((a) <= (b)) { return __runner->failure(__runner, "ASSERT_GT("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }
#define ASSERT_STRNE(a, b, ...) if (strcmp((a), (b)) == 0) { return __runner->failure(__runner, "ASSERT_STRNE("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }
#define ASSERT_LT(a, b, ...) if ((a) >= (b)) { return __runner->failure(__runner, "ASSERT_LT("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }
#define ASSERT_LE(a, b, ...) if ((a) > (b)) { return __runner->failure(__runner, "ASSERT_LE("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }
#define ASSERT_STRCASNE(a, b, ...) if (strcasecmp((a), (b)) == 0) { return __runner->failure(__runner, "ASSERT_STRCASNE("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }
#define ASSERT_GE(a, b, ...) if ((a) < (b)) { return __runner->failure(__runner, "ASSERT_GE("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }
#define ASSERT_STREQ(a, b, ...) if (strcmp((a), (b)) != 0) { return __runner->failure(__runner, "ASSERT_STREQ("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }
#define ASSERT_STRCASEQ(a, b, ...) if (strcasecmp((a), (b)) != 0) { return __runner->failure(__runner, "ASSERT_STRCASEQ("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }
#define ASSERT_EQ(a, b, ...) if ((a) != (b)) { return __runner->failure(__runner, "ASSERT_EQ("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }
#define ASSERT_NE(a, b, ...) if ((a) == (b)) { return __runner->failure(__runner, "ASSERT_NE("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }

#define EXPECT_FALSE(a, ...) if ((a)) {  __runner->error(__runner,  "EXPECT_FALSE("#a")", __FILE__, __LINE__, __VA_ARGS__); }
#define EXPECT_TRUE(a, ...) if (!((a))) {  __runner->error(__runner,  "EXPECT_TRUE("#a")", __FILE__, __LINE__, __VA_ARGS__); }

#define EXPECT_GT(a, b, ...) if ((a) <= (b)) {  __runner->error(__runner, "EXPECT_GT("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }
#define EXPECT_STRNE(a, b, ...) if (strcmp((a), (b)) == 0) {  __runner->error(__runner, "EXPECT_STRNE("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }
#define EXPECT_LT(a, b, ...) if ((a) >= (b)) {  __runner->error(__runner, "EXPECT_LT("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }
#define EXPECT_LE(a, b, ...) if ((a) > (b)) {  __runner->error(__runner, "EXPECT_LE("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }
#define EXPECT_STRCASNE(a, b, ...) if (strcasecmp((a), (b)) == 0) {  __runner->error(__runner, "EXPECT_STRCASNE("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }
#define EXPECT_GE(a, b, ...) if ((a) < (b)) {  __runner->error(__runner, "EXPECT_GE("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }
#define EXPECT_STREQ(a, b, ...) if (strcmp((a), (b)) != 0) {  __runner->error(__runner, "EXPECT_STREQ("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }
#define EXPECT_STRCASEQ(a, b, ...) if (strcasecmp((a), (b)) != 0) {  __runner->error(__runner, "EXPECT_STRCASEQ("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }
#define EXPECT_EQ(a, b, ...) if ((a) != (b)) {  __runner->error(__runner, "EXPECT_EQ("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }
#define EXPECT_NE(a, b, ...) if ((a) == (b)) {  __runner->error(__runner, "EXPECT_NE("#a", "#b")", __FILE__, __LINE__, __VA_ARGS__); }


// === Test Definition Macros === //

#define TEST_F(fixture, test_name) \
	C_TEST_TEST_FIXTURE0(fixture, test_name, C_TEST_STR(fixture), C_TEST_STR(test_name), __FILE__, __LINE__)

#define TEST(namespace, test_name) \
	C_TEST_TEST0(namespace, test_name, C_TEST_STR(namespace), C_TEST_STR(test_name), __FILE__, __LINE__)

// === Runner Macros === //

#define RUN_ALL_TESTS() \
	c_test_run(c_test_create_default_runner())
