/*
 * Copyright (C) 2023 Xiaomi Corperation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __BINDER_INCLUDE_UTILS_STRING_H__
#define __BINDER_INCLUDE_UTILS_STRING_H__

#include <sys/types.h>

#define STRING_INIT_CAPACITY 64

/* TODO: Make String data become growable */

struct String;
typedef struct String String;

struct String {
    char mData[STRING_INIT_CAPACITY];
    size_t mSize;
    size_t mCapacity;
};

static inline const char* String_data(const String* this)
{
    return this->mData;
}

void String_init(String* this, const char* data);
void String_cpyto(String* this, uint8_t* data, size_t len);
size_t String_size(const String* this);
void String_dup(String* this, const String* from);
void String_clear(String* this);
bool String_cmp(const String* str1, const String* str2);

#endif //__BINDER_INCLUDE_UTILS_STRING_H__
