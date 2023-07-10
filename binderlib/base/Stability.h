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

#ifndef __BINDER_INCLUDE_BINDER_STABILITY_H__
#define __BINDER_INCLUDE_BINDER_STABILITY_H__

#include "IBinder.h"
#include "utils/BinderString.h"

enum Level {
    STABILITY_UNDECLARED = 0,
    STABILITY_VENDOR = 0b000011,
    STABILITY_SYSTEM = 0b001100,
    STABILITY_VINTF = 0b111111,
};

enum {
    REPR_NONE = 0,
    REPR_LOG = 1,
    REPR_ALLOW_DOWNGRADE = 2,
};

int16_t Stability_getRepr(IBinder* binder);
int32_t Stability_setRepr(IBinder* binder, int32_t setting,
    uint32_t flags);
void Stability_markCompilationUnit(IBinder* binder);
void Stability_tryMarkCompilationUnit(IBinder* binder);

#endif /* __BINDER_INCLUDE_BINDER_STABILITY_H__ */
