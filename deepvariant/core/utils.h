/*
 * Copyright 2017 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// Core utilities functions used throughout deepvariant.
#ifndef LEARNING_GENOMICS_DEEPVARIANT_CORE_UTILS_H_
#define LEARNING_GENOMICS_DEEPVARIANT_CORE_UTILS_H_

// redacted
#include <map>
#include <type_traits>
#include <vector>

#include "deepvariant/core/genomics/position.pb.h"
#include "deepvariant/core/genomics/range.pb.h"
#include "deepvariant/core/genomics/reads.pb.h"
#include "deepvariant/core/genomics/struct.pb.h"
#include "deepvariant/core/genomics/variants.pb.h"
#include "deepvariant/core/protos/core.pb.h"
#include "tensorflow/core/lib/core/stringpiece.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/platform/logging.h"

namespace learning {
namespace genomics {
namespace core {

using tensorflow::int64;
using tensorflow::string;

// Enum specifying a group of canonical DNA sequence bases.
enum class CanonicalBases {
  // Only allows ACGT bases.
  ACGT,
  // Allows ACGT bases as well as the somewhat standard N base.
  ACGTN,
};

// Returns true if all character in bases are one of canonical bases specified
// in the CanonicalBases canon parameter. If bad_position is not nullptr, we
// will write the position of the first bad base in bases here. If bad_position
// is nullptr nothing will be written to this location.
//
// bases must not be the empty string.
bool AreCanonicalBases(tensorflow::StringPiece bases,
                       CanonicalBases canon = CanonicalBases::ACGT,
                       size_t* bad_position = nullptr);

// Returns true if base is one of canonical bases specified in the
// CanonicalBases canon parameter.
bool IsCanonicalBase(const char base,
                     CanonicalBases canon = CanonicalBases::ACGT);

// Creates a Position proto from chr and pos.
learning::genomics::v1::Position MakePosition(
    tensorflow::StringPiece chr, int64 pos, const bool reverse_strand = false);

// Creates a Position proto from reference_name and start position of Variant.
learning::genomics::v1::Position MakePosition(
    const learning::genomics::v1::Variant& variant);

// Creates a Range proto from chr, start, and end arguments.
learning::genomics::v1::Range MakeRange(tensorflow::StringPiece chr,
                                        int64 start, int64 end);

// Creates a Range proto from the reference_name, start, and end of Variant.
learning::genomics::v1::Range MakeRange(
    const learning::genomics::v1::Variant& variant);

// Creates a Range proto from the alignment of Read.
learning::genomics::v1::Range MakeRange(
    const learning::genomics::v1::Read& read);

// Returns true iff range `needle` is wholly contained in `haystack`.
bool RangeContains(const learning::genomics::v1::Range& haystack,
                   const learning::genomics::v1::Range& needle);

// Creates an interval string from its arguments, like chr:start-end.
string MakeIntervalStr(tensorflow::StringPiece chr, int64 start, int64 end,
                       bool base_zero = true);

// Makes an interval string from a Position proto.
string MakeIntervalStr(const learning::genomics::v1::Position& position);

// Makes an interval string from a Range interval.
string MakeIntervalStr(const learning::genomics::v1::Range& interval);

// Compares pos1 and pos2, lexicographically by reference_name then by position.
int ComparePositions(const learning::genomics::v1::Position& pos1,
                     const learning::genomics::v1::Position& pos2);

// Compares the positions of two Variants via ComparePositions(), so the
// comparison is done on the reference name and start position only.
int ComparePositions(const learning::genomics::v1::Variant& variant1,
                     const learning::genomics::v1::Variant& variant2);

// Returns the contig name to which this read is aligned. Returns empty string
// if the read is unaligned.
string AlignedContig(const learning::genomics::v1::Read& read);

// Get the starting position of read, which is the first base covered
// by cigar operations of read. This is very fast since the start
// is encoded in the read proto.
int64 ReadStart(const learning::genomics::v1::Read& read);

// Gets the end position of the, which is the index of the last base on the
// genome covered by cigar operations in the read. Note this means that the
// end is INCLUSIVE, not exclusive, as many range operations are. Note that
// this operation is substantially more expensive than ReadStart as the
// end must be computed by examining the cigar elements of Read. Implements
// getReferenceLength (excludes padding) as found at:
// http://grepcode.com/file/repo1.maven.org/maven2/org.seqdoop/htsjdk/1.118/htsjdk/samtools/Cigar.java#Cigar.getReferenceLength%28%29
int64 ReadEnd(const learning::genomics::v1::Read& read);

// Returns true if the read is properly placed. We define properly placed as
// read and mate both mapped to the same contig if mapped at all. This is less
// strict than the proper pair SAM flag.
bool IsReadProperlyPlaced(const learning::genomics::v1::Read& read);

// Returns false if Read does not satisfy all of the ReadRequirements.
bool ReadSatisfiesRequirements(const learning::genomics::v1::Read& read,
                               const ReadRequirements& requirements);

// Return a string_view that reflects removing quotation from the ends the
// input.  (e.g. '"foo"' -> "foo"; '\'foo\'' -> 'foo')
// If the input string not quoted (on both sides, using the same quote mark),
// returns the input.
tensorflow::StringPiece Unquote(tensorflow::StringPiece input);

// Creates a mapping from string to int for mapping of contig names to position
// in FASTA. This is used with the CompareVariants function.
std::map<string, int> MapContigNameToPosInFasta(
    const std::vector<core::ContigInfo>& contigs);

// Returns true if Variant `a` should appear before Variant `b`,
// false otherwise.
// Implements Compare to be used in std::sort. See:
// http://en.cppreference.com/w/cpp/concept/Compare
// "The return value of the function call operation applied to an object of "
//  type Compare, when contextually converted to bool, yields true if the
//  first argument of the call appears before the second in the strict weak
//  ordering relation induced by this Compare type, and false otherwise."
bool CompareVariants(const learning::genomics::v1::Variant& a,
                     const learning::genomics::v1::Variant& b,
                     const std::map<string, int>& contig_name_to_pos_in_fasta);

// Returns true if the string s ends with the string t.
bool EndsWith(const string& s, const string& t);

// Templated convenience functions to set a value in a
// learning::genomics::v1::Value based on its C++ type.
//
// These are intended as helper functions for SetInfoField, but can be used
// standalone.
// Sets value to anything that set_string_value accepts as an argument type.
template <typename T>
void SetValuesValue(T value, learning::genomics::v1::Value* protobuf_value) {
  protobuf_value->set_string_value(value);
}

// Sets the value with set_number_value for any arithmetic C++ type.
template <typename T>
void SetValuesValue(
    typename std::enable_if<std::is_arithmetic<T>::value, T>::type value,
    learning::genomics::v1::Value* protobuf_value) {
  protobuf_value->set_number_value(value);
}

// Sets info field map[key] of proto to the vector of values.
//
// This function is heavily templated so that it's easy to use on both Variant
// and VariantCall protos to set the key of their info field maps (of type
// string => google.protobuf.ListValue) to a specific set of values.
//
// Example usages are:
//
//   Variant variant;
//   VariantCall call;
//   SetInfoField(variant, "DP", 10);
//   SetInfoField(call, "AD", vector<int>{10, 20});
//   SetInfoField(variant, "FLOAT_KEY", 1.234);
//   SetInfoField(variant, "STRING_KEY", "a_string");
//
// Note that any existing binding for key will be overwritten.
template <typename ListValueInfoProto, typename Value>
void SetInfoField(const string& key, const std::vector<Value> values,
                  ListValueInfoProto* proto) {
  (*proto->mutable_info())[key].clear_values();
  for (const Value& value : values) {
    SetValuesValue<Value>(value, (*proto->mutable_info())[key].add_values());
  }
}

template <typename ListValueInfoProto, typename Value>
void SetInfoField(const string& key, const Value value,
                  ListValueInfoProto* proto) {
  SetInfoField(key, std::vector<Value>{value}, proto);
}

// Given a ListValue proto, this function makes it easy to get the a vector of
// the number values or string values from that proto, in order. The function is
// templated so one get ints, floats, or strings as needed. Example usages:
//
// vector<int> ints = ListValues<int>(list_value_containing_ints);
// vector<float> floats = ListValues<float>(list_value_containing_floats);
// vector<string> floats = ListValues(list_value_containing_strings);
//
template <typename T>
std::vector<typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
    ListValues(const learning::genomics::v1::ListValue& list_value) {
  std::vector<T> values;
  for (const auto& value : list_value.values()) {
    values.push_back(value.number_value());
  }
  return values;
}

std::vector<string> ListValues(
    const learning::genomics::v1::ListValue& list_value);

}  // namespace core
}  // namespace genomics
}  // namespace learning

#endif  // LEARNING_GENOMICS_DEEPVARIANT_CORE_UTILS_H_
