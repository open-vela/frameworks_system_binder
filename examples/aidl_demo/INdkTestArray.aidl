/*
 * Copyright (C) 2023 Xiaomi Corporation
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

interface INdkTestArray
{
    // Arrays
    boolean[3] RepeatBooleanArray(in boolean[3] input, out boolean[3] repeated);
    byte[3] RepeatByteArray(in byte[3] input, out byte[3] repeated);
    char[3] RepeatCharArray(in char[3] input, out char[3] repeated);
    int[3] RepeatIntArray(in int[3] input, out int[3] repeated);
    long[3] RepeatLongArray(in long[3] input, out long[3] repeated);
    float[3] RepeatFloatArray(in float[3] input, out float[3] repeated);
    double[3] RepeatDoubleArray(in double[3] input, out double[3] repeated);
    String[3] RepeatStringArray(in String[3] input, out String[3] repeated);
}
