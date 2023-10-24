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

interface INdkTestVector
{
    // Arrays
    boolean[] RepeatBooleanVector(in boolean[] input, out boolean[] repeated);
    byte[] RepeatByteVector(in byte[] input, out byte[] repeated);
    char[] RepeatCharVector(in char[] input, out char[] repeated);
    int[] RepeatIntVector(in int[] input, out int[] repeated);
    long[] RepeatLongVector(in long[] input, out long[] repeated);
    float[] RepeatFloatVector(in float[] input, out float[] repeated);
    double[] RepeatDoubleVector(in double[] input, out double[] repeated);
    String[] RepeatStringVector(in String[] input, out String[] repeated);
    String[] RepeatInOutStringVector(in String[] input, inout String[] repeated);

    // List
    List<String> Repeat2StringList(in List<String> input, out List<String> repeated);
    @nullable List<String> Repeat2NullableStringList(in @nullable List<String> input, out @nullable List<String> repeated);
    List<String> Repeat2InOutStringList(in List<String> input, inout List<String> repeated);
    @nullable List<String> Repeat2InOutNullableStringList(in @nullable List<String> input, inout @nullable List<String> repeated);

    // Nullable Arrays
    @nullable boolean[] RepeatNullableBooleanVector(in @nullable boolean[] input);
    @nullable byte[] RepeatNullableByteVector(in @nullable byte[] input);
    @nullable char[] RepeatNullableCharVector(in @nullable char[] input);
    @nullable int[] RepeatNullableIntVector(in @nullable int[] input);
    @nullable long[] RepeatNullableLongVector(in @nullable long[] input);
    @nullable float[] RepeatNullableFloatVector(in @nullable float[] input);
    @nullable double[] RepeatNullableDoubleVector(in @nullable double[] input);
    @nullable String[] RepeatNullableStringVector(in @nullable String[] input);
    @nullable String[] RepeatInOutNullableStringVector(in @nullable String[] input, inout @nullable String[] repeated);
}
