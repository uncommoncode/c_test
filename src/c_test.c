#include "c_test.h"

// FAQ
// ---
//  Q: Why "__runner" and not "runner__"?
//    A: I didn't want to pollute any autocomplete for an argument that typically should not be used.
//
//  Q: Why not GoogleTest?
//    A: GoogleTest is fantastic, but by design requires C++. This only requires C with support for the constructor 
//       attribute supported in GCC and Clang. This makes it more appropriate for embedded environments and toolchains 
//       you dont have as much flexibility over requiring C+GCC.
//
//  Q: Why not export JSON or XML?
//    A: I want to keep things simple here and instead expose the runner_t interface allowing users to customize their 
//       output. However I would welcome additions here!
//
//  Q: This is a lot of macro code. Did you generate it all by hand?
//    A: No. I generated it with python.
//
//  Q: Why force a format string?
//    A: This was tough, but it's generally a good practice in unit tests to document the failure cases. In the worst 
//       case you can just write ASSERT_TRUE(<expression>, ""); This seems ideal versus other options like function 
//       chaining where it's not clear where the chain ends. For example, having code like 
//       ASSERT_NE(data, NULL)("data failed to initialize") or 
//       ASSERT_NE(data, NULL)->message("data failed to initialize") is beautiful in it's own way, but adds overhead to 
//       the implementation complexity to buffer around values until either the test finishes executing or the test 
//       completes without any significant reduction in boilerplate.  

#include <string.h>

#ifdef C_TEST_USE_PRINTF_RUNNER
#include <stdio.h>
#include <stdarg.h>

#ifdef C_TEST_USE_PRINTF_TIMER
#include <sys/time.h>
#endif
#endif

// === Thin Vector-like struct === //

const int kDefaultVectorCapacity = 32;

typedef struct {
	void *data;

	uint32_t count;
	uint32_t capacity;
	uint32_t element_size;
} c_test_vector_t;

void c_test_vector_init(c_test_vector_t *vector, uint32_t element_size, uint32_t capacity) {
	vector->count = 0;
	vector->element_size = element_size;
	vector->capacity = capacity;
	vector->data = malloc(element_size * capacity);
}

void c_test_vector_destroy(c_test_vector_t *vector) {
	if (NULL != vector->data) {
		free(vector->data);
		vector->data = NULL;
		vector->count = 0;
		vector->element_size = 0;
		vector->capacity = 0;
	}
}

void c_test_vector_push_back(c_test_vector_t *vector, const void *value) {
	if (vector->count >= vector->capacity) {
		vector->capacity = 2 * vector->capacity;
		vector->data = realloc(vector->data, vector->capacity * vector->element_size);
	}
	memcpy(vector->data + vector->count * vector->element_size, value, vector->element_size);
	vector->count++;
}


// === Default runner implementation printing output to stdout === //

#ifdef C_TEST_USE_PRINTF_RUNNER

#define CONSOLE_COLOR_RED "\x1b[31m"
#define CONSOLE_COLOR_GREEN "\x1b[32m"
#define CONSOLE_COLOR_RESET "\x1b[0m"

typedef struct {
	uint64_t test_start_ms;
	c_test_vector_t failed_test_names;
} print_data_t;

void print_failure(c_test_runner_t* runner, const char *expression, const char *file_name, int line_number,
				   const char *format, ...) {
	printf("%s(%d): error: ", file_name, line_number);

	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	printf("\n");

	printf("%s failed\n", expression);
	
	runner->error_count++;
}

void print_error(c_test_runner_t* runner, const char *expression, const char *file_name, int line_number, 
				 const char *format, ...) {
	printf("%s(%d): error: ", file_name, line_number);

	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	printf("\n");

	printf("%s failed\n", expression);

	runner->error_count++;
}

void print_success(c_test_runner_t* runner, const char *file_name, int line_number) {
}

void print_begin_tests(c_test_runner_t* runner, uint32_t test_count) {
	printf(CONSOLE_COLOR_GREEN "[----------] " CONSOLE_COLOR_RESET);
	printf("Running %d tests.\n", test_count);
	printf("\n");
}

void print_end_tests(c_test_runner_t* runner, uint32_t test_count, uint32_t failed_count) {
	printf("\n");
	printf(CONSOLE_COLOR_GREEN "[----------] " CONSOLE_COLOR_RESET);
	printf("Completed %d tests with %d failures\n", test_count, failed_count);

	if (failed_count != 0) {
		print_data_t *print_data = runner->data;

		const char **failed_test_names = print_data->failed_test_names.data;
		for (int i = 0; i < print_data->failed_test_names.count; i++) {
			const char *failed_test_name = failed_test_names[i];
			printf(CONSOLE_COLOR_RED "[  FAILED  ] " CONSOLE_COLOR_RESET);
			printf("%s\n", failed_test_name);
		}
	}
}

#ifdef C_TEST_USE_PRINTF_TIMER
uint64_t get_current_ms() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * (uint64_t)1000 + (tv.tv_usec) / 1000;
}
#else
uint64_t get_current_ms() {
	return 0;
}
#endif

void print_begin_test(c_test_runner_t* runner) {
	printf(CONSOLE_COLOR_GREEN "[ RUN      ] " CONSOLE_COLOR_RESET);
	printf("%s.%s\n", runner->current_test->name_space, runner->current_test->test_name);
	print_data_t *print_data = runner->data;
	print_data->test_start_ms = get_current_ms();
}

void print_end_test(c_test_runner_t* runner, int test_success) {
#ifdef C_TEST_USE_PRINTF_TIMER
	const uint64_t test_end_ms = get_current_ms();
#endif
	print_data_t *print_data = runner->data;
	if (test_success) {
		printf(CONSOLE_COLOR_GREEN "[       OK ] " CONSOLE_COLOR_RESET);
	} else {
		printf(CONSOLE_COLOR_RED "[  FAILED  ] " CONSOLE_COLOR_RESET);
		// add one for dot, add one for null termination
		int test_name_size = strlen(runner->current_test->name_space) + strlen(runner->current_test->test_name) + 2;
		char *failed_test_name = (char*)malloc(sizeof(char) * test_name_size);
		snprintf(failed_test_name, test_name_size, "%s.%s", runner->current_test->name_space, 
				 runner->current_test->test_name);
		c_test_vector_push_back(&print_data->failed_test_names, &failed_test_name);
	}
#ifdef C_TEST_USE_PRINTF_TIMER
	uint64_t duration_ms = test_end_ms - print_data->test_start_ms;
	printf("%s.%s (%llu ms)\n", runner->current_test->name_space, runner->current_test->test_name, duration_ms);
#else
	printf("%s.%s (? ms)\n", runner->current_test->name_space, runner->current_test->test_name);
#endif
}

void print_destroy(c_test_runner_t *runner) {
	print_data_t *print_data = runner->data;

	if (NULL == print_data) {
		return;
	}

	char **failed_test_names = print_data->failed_test_names.data;
	for (int i = 0; i < print_data->failed_test_names.count; i++) {
		free(failed_test_names[i]);
	}
}

c_test_runner_t* c_test_create_default_runner() {
	static print_data_t print_data = {
		.test_start_ms = 0,
	};
	c_test_vector_init(&print_data.failed_test_names, sizeof(char*), kDefaultVectorCapacity);
	static c_test_runner_t runner = {
		.failure = print_failure,
		.success = print_success,
		.error = print_error,

		.begin_tests = print_begin_tests,
		.end_tests = print_end_tests,

		.begin_test = print_begin_test,
		.end_test = print_end_test,
		
		.destroy = print_destroy,

		.error_count = 0,
		.data = &print_data,
		.current_test = NULL,
	};
	return &runner;
}

#else

void silent_failure(c_test_runner_t* runner, const char *expression, const char *file_name, int line_number, 
					const char *format, ...) {
	runner->error_count++;
}

void silent_error(c_test_runner_t* runner, const char *expression, const char *file_name, int line_number, 
				  const char *format, ...) {
	runner->error_count++;
}

void silent_success(c_test_runner_t* runner, const char *file_name, int line_number) {
}

c_test_runner_t* c_test_create_default_runner() {
	static c_test_runner_t runner = {
		.failure = silent_failure,
		.success = silent_success,
		.error = silent_error,

		.begin_test = NULL,
		.end_test = NULL,

		.begin_tests = NULL,
		.end_tests = NULL,

		.destroy = NULL,

		.error_count = 0,
		.data = NULL,
		.current_test = NULL,
	};
	return &runner;
}
#endif

// === Global context === //

typedef struct {
	int initialized;
	c_test_vector_t test_definitions;
} c_test_context_t;

c_test_context_t* c_test_get_context() {
	static c_test_context_t context = {.initialized = 0};
	if (!context.initialized) {
		c_test_vector_init(&context.test_definitions, sizeof(c_test_definition_t), kDefaultVectorCapacity);

		context.initialized = 1;
	}
	return &context;
}


// === Adding tests === //

void c_test_add_test_fixture(c_test_fixture_t fixture, c_test_fixture_function_t function, const char *fixture_name, 
					  const char *test_name, const char *file_name, int line_number) {
	c_test_context_t *context = c_test_get_context();

	c_test_definition_t test_definition = {
		.setup = fixture.setup,
		.teardown = fixture.teardown,
		
		.name_space = fixture_name,
		.test_name = test_name,

		.file_name = file_name,
		.line_number = line_number,

		.test_function = NULL,
		.test_fixture_function = function,
	};

	c_test_vector_push_back(&context->test_definitions, &test_definition);
}

void c_test_add_test(c_test_function_t function, const char *name_space, const char *test_name, const char *file_name, 
			  int line_number) {
	c_test_context_t *context = c_test_get_context();

	c_test_definition_t test_definition = {
		.setup = NULL,
		.teardown = NULL,
		
		.name_space = name_space,
		.test_name = test_name,

		.file_name = file_name,
		.line_number = line_number,

		.test_function = function,
		.test_fixture_function = NULL,
	};

	c_test_vector_push_back(&context->test_definitions, &test_definition);
}


// === Running tests === //

void c_test_run_test(c_test_runner_t *runner, void *data) {
	if (NULL != runner->current_test->test_function) {
		runner->current_test->test_function(runner);
	} else if (NULL != runner->current_test->test_fixture_function) {
		runner->current_test->test_fixture_function(runner, data);
	} else {
		runner->failure(runner, "", __FILE__, __LINE__, "No valid test function for %s:%d", 
						runner->current_test->file_name, runner->current_test->line_number);
	}
}

int c_test_run(c_test_runner_t* runner) {
	c_test_context_t *context = c_test_get_context();

	const c_test_definition_t *test_definitions = (const c_test_definition_t *)context->test_definitions.data;

	if (NULL != runner->begin_tests) {
		runner->begin_tests(runner, context->test_definitions.count);
	}

	uint32_t failed_test_count = 0;
	for (int i = 0; i < context->test_definitions.count; i++) {
		c_test_setup_function_t setup_function = test_definitions[i].setup;
		c_test_teardown_function_t teardown_function = test_definitions[i].teardown;
		void *data_ = NULL;

		runner->current_test = test_definitions + i;

		if (NULL != setup_function) {
			data_ = setup_function();
		}
		
		const uint32_t start_error_count = runner->error_count;

		if (NULL != runner->begin_test) {
			runner->begin_test(runner);
		}

		c_test_run_test(runner, data_);

		const int test_success = start_error_count == runner->error_count;

		if (!test_success) {
			failed_test_count++;
		}

		if (NULL != runner->end_test) {
			runner->end_test(runner, test_success);
		}
		
		if (NULL != teardown_function) {
			teardown_function(data_);
		}

		runner->current_test = NULL;
	}

	if (NULL != runner->end_tests) {
		runner->end_tests(runner, context->test_definitions.count, failed_test_count);
	}

	int return_code = failed_test_count == 0 ? 0 : 1;
	if (NULL != runner->destroy) {
		runner->destroy(runner);
	}
	return return_code;
}
