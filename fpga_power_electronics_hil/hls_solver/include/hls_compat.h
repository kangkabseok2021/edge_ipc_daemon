#pragma once
// On host / CI: compile HLS C++ as regular C++ with type aliases.
// Under Vitis HLS synthesis (__SYNTHESIS__ is defined automatically):
//   ap_fixed<32,16>, ap_uint<6>, hls::stream<T> come from Vitis headers.
#ifndef __SYNTHESIS__
#include <cstdint>
#include <cmath>

// ap_fixed<32,16>: 32-bit fixed-point, 16 integer bits.
// On host we use double so formulae remain readable.
using ap_fixed_32_16 = double;
using ap_uint_6      = uint8_t;
using ap_uint_32     = uint32_t;

// No-op HLS pragma shim — keeps pragmas in source without breaking host compile
#define HLS_PRAGMA(x)

#else
// Real Vitis HLS headers (available inside Vitis HLS tool only)
#include <ap_fixed.h>
#include <ap_int.h>
using ap_fixed_32_16 = ap_fixed<32, 16>;
using ap_uint_6      = ap_uint<6>;
using ap_uint_32     = ap_uint<32>;
#define HLS_PRAGMA(x) _Pragma(#x)
#endif
