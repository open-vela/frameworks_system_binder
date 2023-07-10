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

#include <debug.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <nuttx/arch.h>
#include <nuttx/clock.h>
#include <nuttx/init.h>

#include "utils/BinderString.h"

void String_init(String* this, const char* data)
{
    this->mCapacity = 64;
    if (data != NULL) {
        strncpy(this->mData, data, this->mCapacity - 1);
        this->mSize = strlen(data);
    } else {
        memset(this->mData, 0, STRING_INIT_CAPACITY);
        this->mSize = 0;
    }
}

void String_cpyto(String* this, uint8_t* data, size_t len)
{
    memcpy(data, this->mData, len);
}

void String_dup(String* this, const String* from)
{
    memcpy(this->mData, from->mData, from->mSize);
    this->mSize = from->mSize;
}

size_t String_size(const String* this)
{
    return this->mSize;
}

void String_clear(String* this)
{
    String_init(this, NULL);
}

bool String_cmp(const String* str1, const String* str2)
{
    return strcmp(String_data(str1), String_data(str2));
}
