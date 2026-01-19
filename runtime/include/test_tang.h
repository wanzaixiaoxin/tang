#pragma once

#include <stddef.h>

// Simple enum definition
enum TestEnum {
    TEST_VALUE1,
    TEST_VALUE2,
    TEST_VALUE3
};

// Simple struct definition
typedef struct TestStruct {
    int value;
    void* ptr;
} TestStruct;

// Simple function declarations
typedef void (*TestFunc)(void*);
TestStruct* test_create(int value);
void test_destroy(TestStruct* obj);
int test_get_value(const TestStruct* obj);
