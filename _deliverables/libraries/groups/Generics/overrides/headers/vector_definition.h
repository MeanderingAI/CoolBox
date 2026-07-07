#pragma once

#include <vector>

template<typename V>
using v = std::vector<V>;

#include "b.inc"
#include "uni.inc"
#include "to.inc"
#include "m.inc"
#include "acl.inc"
#include "sh.inc"

template<typename I>
using standard_puts = M<I>;