#include <stdlib.h>
#include "../include/test_tang.h"

TestStruct* test_create(int value) {
    TestStruct* obj = (TestStruct*)malloc(sizeof(TestStruct));
    if (!obj) {
        return NULL;
    }
    obj->value = value;
    obj->ptr = NULL;
    return obj;
}

void test_destroy(TestStruct* obj) {
    if (obj) {
        free(obj);
    }
}

int test_get_value(const TestStruct* obj) {
    if (!obj) {
        return -1;
    }
    return obj->value;
}
