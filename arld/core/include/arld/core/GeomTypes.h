#pragma once
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>

namespace arld::core {

using Kernel   = CGAL::Exact_predicates_inexact_constructions_kernel;
using Point2   = Kernel::Point_2;
using Segment2 = Kernel::Segment_2;
using Polygon2 = CGAL::Polygon_2<Kernel>;
using PolySet  = CGAL::Polygon_with_holes_2<Kernel>;
using FT       = Kernel::FT;

} // namespace arld::core
