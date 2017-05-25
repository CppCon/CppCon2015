// Program: Testing and exploring IEEE floating-point arithmentic
//          with C++ standard algorithms.
// Author: Lawrence Crowl


// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
//
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// For more information, please refer to <http://unlicense.org/>


#include <assert.h>
#include <stdio.h>
#include <cstdint>
#include <string.h>
#include <vector>
#include <algorithm>
#include <unordered_map>

using namespace std;


//////////// numeric_limits

void report_numeric_limits() {
  printf( "\nNumeric Limits\n" );
  printf( "has_infinity %d\n", numeric_limits<double>::has_infinity );
  printf( "has_quiet_NaN %d\n", numeric_limits<double>::has_quiet_NaN );
  printf( "has_signaling_NaN %d\n", numeric_limits<double>::has_signaling_NaN );
  printf( "has_denorm %d\n", numeric_limits<double>::has_denorm );
  printf( "has_denorm_loss %d\n", numeric_limits<double>::has_denorm_loss );
}


//////////// representation

// For the efficiency of some operations,
// we need access to the representation of the floating-point value.
// It is best when that representation is signed,
// as that makes sign testing faster.
// The memcpy operations are necessary to avoid type aliasing,
// and compilers should eliminate them.

typedef int64_t bits;
static_assert( sizeof (bits) == sizeof (double), "unexpected double size" );

bits rep( double d ) {
  bits b;
  unsigned char m[ sizeof (double) ];
  memcpy( m, &d, sizeof (double) );
  memcpy( &b, m, sizeof (double) );
  return b;
}

double dbl( bits b ) {
  double d;
  unsigned char m[ sizeof (double) ];
  memcpy( m, &b, sizeof (double) );
  memcpy( &d, m, sizeof (double) );
  return d;
}

// Table of IEEE floating-point values adapted from
// http://steve.hollasch.net/cgindex/coding/ieeefloat.html
// from highest total order to lowest:

// sign exponent significand value
// 0    11...11  11...11     max positive quiet NaN
// 0    11...11  10...00     min positive quiet NaN
// 0    11...11  01...11     max positive signaling NaN
// 0    11...11  00...01     min positive signaling NaN
// 0    11...11  00...00     positive infinity
// 0    01...11  11...11     max positive normalized real
// 0    00...01  00...00     min positive normalized real
// 0    00...00  11...11     max positive denormalized real
// 0    00...00  00...01     min positive denormalized real
// 0    00...00  00...00     positive zero
// 1    00...00  00...00     negative zero
// 1    00...00  00...01     max negative normalized real
// 1    00...00  11...11     min negative denormalized real
// 1    00...01  00...00     max negative normalized real
// 1    01...11  11...11     min negative normalized real
// 1    11...11  00...00     negative infinity
// 1    11...11  00...01     max negative signaling NaN
// 1    11...11  01...11     min negative signaling NaN
// 1    11...11  10...00     max negative quiet NaN
// 1    11...11  11...11     min negative quiet NaN

//////////// utilities

const int exponent_width = 11;
const int header_width = exponent_width+1;
const int significand_width = 52;
const int payload_width = significand_width-1;

constexpr bits mask( int n /* >0 */ ) {
  return ~(~static_cast<bits>(0) << n);
}

bool is_minus( double d ) {
  return rep(d) < 0; // most significant bit is set.
}

bits its_exponent( double a ) {
  return (rep(a) >> significand_width) & mask(exponent_width);
}

bits its_significand( double d ) {
  return rep(d) & mask(significand_width);
}

bits its_header( double d ) {
  return (rep(d) >> significand_width) & mask(header_width);
}

bits is_NaN( double d ) {
  bool a = isnan(d);
  bool b = d != d; // The convention as defined by IEEE.
  assert( a == b );
  return b;
}

bits is_positive_NaN( double d ) {
  bool a = is_NaN(d) && ! is_minus(d);
  bool b = rep(d) > 9218868437227405312; // 0x7FF0000000000000
  bool c = its_header(d) == 0x7FF && its_significand(d) > 0;
  assert( a == b && b == c );
  return b; // Seems most efficient.
}

bits is_negative_NaN( double d ) {
  bool a = is_NaN(d) && is_minus(d);
  bool b = rep(d) < 0 && rep(d) > -4503599627370496; // 0xFFF0000000000000
  bool c = its_header(d) == 0xFFF && its_significand(d) > 0;
  assert( a == b && b == c );
  return a; // Seems most efficient.
}

bits its_payload( double d ) { // Assuming d is a NaN.
  return rep(d) & mask(payload_width);
}

bool is_quiet( double d ) { // Assuming d is a NaN.
  return ((rep(d) >> payload_width) & mask(1)) != 0;
}

double make_Inf() {
   bits b = (0x7FFll << significand_width);
   return dbl( b );
}

double make_qNaN( bits payload ) {
   bits b = (0x7FFll << significand_width) | (1ll << payload_width)
            | (payload & mask(payload_width));
   return dbl( b );
}

double make_sNaN( bits payload ) {
   assert( (payload & mask(payload_width)) != 0 );
   bits b = (0x7FFll << significand_width)
            | (payload & mask(payload_width));
   double d = dbl( b );
   // NOTE: On x87, this signaling NaN will almost immediately become quiet.
   return d;
}


//////////// constants

const double Inf = make_Inf();
const double sNaN = make_sNaN(1);
const double qNaN = make_qNaN(0);


//////////// printing and dumping

void print( double d ) {
  if ( is_minus(d) ) {
    printf( "-" );
    d = -d;
  }
  else {
    printf( "+" );
  }
  if ( is_NaN(d) ) {
    if ( is_quiet(d) )
      printf( "qNaN:" );
    else
      printf( "sNaN:" );
    bits payload = its_payload(d);
    printf( "%llX", payload );
  } else {
    printf( "%g", d );
  }
}

void xray( double d ) {
  if ( is_minus(d) ) {
    printf( "-" );
    d = -d;
  }
  else {
    printf( "+" );
  }
  printf( "%03llX:%013llX", its_exponent(d), its_significand(d) );
}

void dump( double d ) {
  printf( "%18f ", d );
  printf( "%016llX ", rep(d) );
  xray(d);
  printf( " " );
  print(d);
}

void dumpl( double d ) {
  dump(d);
  printf( "\n" );
}


//////////// selected dumping

void dump_selected_values() {
  printf( "\nSelected Values\n" );
  dumpl(-0.0);
  dumpl(+0.0);
  dumpl(-1.0);
  dumpl(+1.0);
  dumpl(-2.2);
  dumpl(+2.2);
  dumpl(-Inf);
  dumpl(+Inf);
  dumpl(sNaN);
  dumpl(-sNaN);
  dumpl(+sNaN);
  dumpl(qNaN);
  dumpl(-qNaN);
  dumpl(+qNaN);
}


//////////// order comparators

enum class partial_ordering { less, unordered, greater };
partial_ordering partial_order( double d, double e ) {
  if ( d < e ) return partial_ordering::less;
  if ( d > e ) return partial_ordering::greater;
  return partial_ordering::unordered;
}

enum class weak_ordering { less, equivalent, greater };
weak_ordering weak_order( double d, double e ) {
  // Evaluate the common cases first.
  if ( d <  e ) return weak_ordering::less;
  if ( d >  e ) return weak_ordering::greater;
  if ( d == e ) return weak_ordering::equivalent;
  // None of the above are true, so at least one of a and b is NaN.
  // The kind of NaN is immaterial, but sign is material.
  if ( is_negative_NaN(d) ) {
    if ( is_negative_NaN(e) ) return weak_ordering::equivalent;
    else return weak_ordering::less;
  }
  else if ( is_positive_NaN(e) ) {
    if ( is_positive_NaN(d) ) return weak_ordering::equivalent;
    else return weak_ordering::less;
  }
  else {
    // Either is_positive_NaN(d) or is_negative_NaN(e),
    // but they cannot be the same.
    return weak_ordering::greater;
  }
}

enum class total_ordering { less, equal, greater };
total_ordering total_order( double d, double e ) {
  // Evaluate the common cases first.
  if ( d < e ) return total_ordering::less;
  if ( d > d ) return total_ordering::greater;
  // Switch to comparison on the representation.
  // It must handle sNaN and signed zeros.
  if ( is_minus(d) && is_minus(e) ) {
    // Both values are negative.
    // IEEE uses signed magnitude, which means the sense of comparisons on
    // negatives are reversed from the two's complement.
    if ( rep(d) > rep(e) ) return total_ordering::less;
    if ( rep(d) < rep(e) ) return total_ordering::greater;
    return total_ordering::equal;
  }
  else {
    // Since either d or e is positive,
    // it does not matter which negative value the other might have.
    if ( rep(d) < rep(e) ) return total_ordering::less;
    if ( rep(d) > rep(e) ) return total_ordering::greater;
    return total_ordering::equal;
  }
}


//////////// order relations

bool partial_less_actual( double d, double e ) {
  // The operator< is a partial relation.
  return d < e;
}

bool partial_less( double d, double e ) {
  bool r = partial_less_actual(d, e);
  bool q = partial_ordering::less == partial_order(d, e);
  assert( r == q );
  return r;
}

bool weak_less_actual( double d, double e ) {
  // Evaluate the common cases first.
  if ( d <  e ) return true;
  if ( d >= e ) return false;
  // At least one of a and b is NaN.
  // The kind of NaN is immaterial, but sign is material.
  if ( is_minus(d) )
    return ! is_negative_NaN(e);
  else
    return ! is_positive_NaN(d) && is_positive_NaN(e);
}

bool weak_less( double d, double e ) {
  bool r = weak_less_actual(d, e);
  bool q = weak_ordering::less == weak_order(d, e);
  assert( r == q );
  return r;
}

bool total_less_actual( double d, double e ) {
  // Evaluate the common cases first.
  if ( d < e ) return true;
  if ( d > e ) return false;
  // Switch to comparison on the representation.
  // It must handle sNaN and signed zeros.
  if ( is_minus(d) && is_minus(e) ) {
    // Both values are negative.
    // IEEE uses signed magnitude, which means the sense of comparisons on
    // negatives are reversed from the two's complement.
    return rep(d) > rep(e);
  }
  else {
    // Since either d or e is positive,
    // it does not matter which negative value the other might have.
    return rep(d) < rep(e);
  }
}

bool total_less( double d, double e ) {
  bool r = total_less_actual(d, e);
  bool q = total_ordering::less == total_order(d, e);
  assert( r == q );
  return r;
}


//////////// equivalence relations

bool builtin( double d, double e ) {
  return d == e; // IEEE operator== IS NEITHER AN EQUIVALENCE NOR AN EQUALITY!
}

bool partial_unordered_actual( double d, double e ) {
  return ! (partial_less(d, e) || partial_less(e, d));
}

bool partial_unordered( double d, double e ) {
  bool r = partial_unordered_actual(d, e);
  bool q = partial_ordering::unordered == partial_order(d, e);
  assert( r == q );
  return r;
}

bool weak_equal_actual( double d, double e ) {
  // -NaN is not equivalent to +NaN.
  return d == e || (is_NaN(d) && is_NaN(e) && is_minus(d) == is_minus(e));
}

bool weak_equal( double d, double e ) {
  bool r = weak_equal_actual(d, e);
  bool q = weak_ordering::equivalent == weak_order(d, e);
  assert( r == q );
  return r;
}

bool total_equal_actual( double d, double e ) {
  return rep(d) == rep(e);
}

bool total_equal( double d, double e ) {
  bool r = total_equal_actual(d, e);
  bool q = total_ordering::equal == total_order(d, e);
  assert( r == q );
  return r;
}


//////////// example sorting

void print(vector<double>& r) {
  for ( double x: r ) {
#if 1
    printf( " " );
    print( x );
#else
    printf( " %g", x );
#endif
  }
  printf( "\n" );
}

//////////// example sorting

void showsort(vector<double> v) {
  sort( v.begin(), v.end() );
  print( v );
}

void showsort(vector<double> v, bool compare(double,double) ) {
  sort( v.begin(), v.end(), compare );
  print( v );
}

void sort_example(vector<double> v) {
  printf( "\n" );
  printf( "example" );
  print(v);
  printf( "default" );
  showsort(v);
  printf( "partial" );
  showsort(v, partial_less);
  printf( "weak   " );
  showsort(v, weak_less);
  printf( "total  " );
  showsort(v, total_less);
}

void show_plain_sort_examples() {
  printf( "\nPlain Sort\n" );
  sort_example( { +Inf, -Inf, -qNaN, +qNaN, +10.0, -10.0,
                  +1.0, -1.0, +sNaN, -sNaN, +0.0, -0.0  } );
}

void show_NaN_sort_examples() {
  printf( "\nNaN Sort\n" );
  sort_example( { -sNaN, -1.0, +0.0, +1.0, +qNaN } );
  sort_example( { +qNaN, +1.0, +0.0, -1.0, -sNaN } );
  sort_example( { -1.0, -sNaN, +0.0, +qNaN, +1.0 } );
  sort_example( { +1.0, -sNaN, +0.0, +qNaN, -1.0 } );
  sort_example( { -1.0, -sNaN, +1.0, +qNaN, +0.0 } );
  sort_example( { +qNaN, -sNaN, +1.0, -1.0, +0.0 } );
}

void show_zero_sort_examples() {
  printf( "\nZero Sort\n" );
  sort_example( { -1.0, -0.0, +0.0, -0.00, +1.0 } );
  sort_example( { +1.0, -0.0, +0.0, -0.00, -1.0 } );
  sort_example( { -1.0, -0.0, +1.0, -0.00, +0.0 } );
  sort_example( { +0.0, -0.0, +1.0, -0.00, -1.0 } );
}

//////////// memoization examples

size_t doublehash( double d ) {
#if 0
  bits hash = rep(d);
  size_t shift_count = sizeof (bits) * 4;
  while ( shift_count > 0 ) {
    hash = hash ^ (hash >> shift_count) ^ (hash << shift_count);
    shift_count /= 2;
  }
  return hash;
#else
  return 0;
#endif
}

struct hash_memoizer
{
  typedef std::unordered_map< double, double,
				size_t(*)(double),
				bool(*)(double,double) > storetype;
  storetype store;
  hash_memoizer( bool (*equiv)(double,double) )
    : store( 30, doublehash, equiv ) { }
  double compute( double d, double (*func)(double) ) {
    storetype::iterator i = store.find(d);
    if ( i == store.end() ) {
      double& ref = store[d];
      ref = func(d);
      return ref;
    }
    else {
      return store[d];
    }
  }
};

void showmemo(vector<double> v,
              bool(*equiv)(double,double), double(*func)(double) ) {
  hash_memoizer memo(equiv);
  for ( double x : v ) {
    printf( " " );
    print( memo.compute( x, func ) );
  }
  printf( "\n" );
}

void memo_example(vector<double> v, double(*func)(double) ) {
  printf( "\n" );
  printf( "example   " );
  print(v);
  printf( "no memoing" );
  for ( double x : v ) {
    printf( " " );
    print( func(x) );
  }
  printf( "\n" );
  printf( "builtin   " );
  showmemo(v, builtin, func);
  printf( "partial   " );
  showmemo(v, builtin, func);
  printf( "weak      " );
  showmemo(v, weak_equal, func);
  printf( "total     " );
  showmemo(v, total_equal, func);
}

double hidden(double d) {
  return its_payload(d);
}

double invert(double d) {
  return 1.0/d;
}

void show_NaN_memo_examples() {
  printf( "\nNaN Memo\n" );
  double q1 = make_qNaN(1);
  double q2 = make_qNaN(2);
  double s1 = make_sNaN(1);
  double s3 = make_sNaN(3);
  double zp = +0.0;
  memo_example( { q1, q2, zp, s3, s1 }, invert );
  memo_example( { s3, q2, s1, q1, zp }, invert );
  memo_example( { zp, q2, s1, s3, q1 }, invert );
  memo_example( { q2, s1, zp, s3, q1 }, invert );
  memo_example( { q2, s1, zp, s3, q1 }, invert );
}

void show_zero_memo_examples() {
  printf( "\nZero Memo\n" );
  memo_example( { -1.0, -0.0, +0.0, -0.00, +1.0 }, invert );
  memo_example( { +1.0, -0.0, +0.0, -0.00, -1.0 }, invert );
  memo_example( { -1.0, -0.0, +1.0, -0.00, +0.0 }, invert );
  memo_example( { +0.0, -0.0, +1.0, -0.00, -1.0 }, invert );
}


//////////// main function

int main() {
  printf( "\n" );
  report_numeric_limits();
  dump_selected_values();
  show_plain_sort_examples();
  show_NaN_sort_examples();
  show_zero_sort_examples();
  show_NaN_memo_examples();
  show_zero_memo_examples();
  printf( "\n" );
}

