#include "openlr/openlr_simple_decoder.hpp"
#include "openlr/openlr_simple_parser.hpp"

#include "routing/car_model.hpp"
#include "routing/car_router.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/router_delegate.hpp"

#include "indexer/index.hpp"
#include "indexer/scales.hpp"

#include "geometry/polyline2d.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"

#include "geometry/latlon.hpp"

#include "std/algorithm.hpp"
#include "std/fstream.hpp"

// double constexpr kMwmRoadCrossingRadiusMeters = 2.0;
// m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(cross, kMwmRoadCrossingRadiusMeters);
//   m_index.ForEachInRect(featuresLoader, rect, GetStreetReadScale());
// }

namespace  // A staff to get road data.
{
vector<m2::PointD> GetFeaturePoints(FeatureType const & ft)
{
  vector<m2::PointD> points;
  points.reserve(ft.GetPointsCount());
  ft.ForEachPoint([&points](m2::PointD const & p)
                  {
                    points.push_back(p);
                  }, scales::GetUpperScale());
  return points;
}

double GetDistanceToLinearFeature(m2::PointD const & p, FeatureType const & ft)
{
  m2::PolylineD poly(GetFeaturePoints(ft));
  return sqrt(poly. GetShortestSquareDistance(p));
}

vector<FeatureType> GetRoadFeaturesAtPoint(Index const & index,
                                           routing::FeaturesRoadGraph & roadGraph,
                                           ms::LatLon const latLon)
{
  vector<FeatureType> features;

  auto const center = MercatorBounds::FromLatLon(latLon);
  auto const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(center, 25);
  ASSERT_LESS_OR_EQUAL(MercatorBounds::DistanceOnEarth(rect.LeftTop(), rect.LeftBottom()), 50.1, ());
  ASSERT_LESS_OR_EQUAL(MercatorBounds::DistanceOnEarth(rect.RightTop(), rect.RightBottom()), 50.1, ());
  ASSERT_LESS_OR_EQUAL(MercatorBounds::DistanceOnEarth(rect.LeftTop(), rect.RightTop()), 50.1, ());
  ASSERT_LESS_OR_EQUAL(MercatorBounds::DistanceOnEarth(rect.LeftBottom(), rect.RightBottom()), 50.1, ());

  index.ForEachInRect([&features, &roadGraph, &center](FeatureType const & ft) {
      ft.ParseHeader2();
      if (!roadGraph.IsRoad(ft))
        return;

      // TODO(mgsergio): Parse only necessary fields.
      ft.ParseEverything();

      if (ft.GetPointsCount() == 1)
      {
        LOG(LDEBUG, ("A linear feature with one"));
        return;
      }

      auto constexpr kMaxDistanceMeters = 10.0;
      // if (GetDistanceToLinearFeature(center, ft) >= kMaxDistanceMeters)
      //   return;

      auto good = false;
      for (auto const & p : GetFeaturePoints(ft))
      {
        if (MercatorBounds::DistanceOnEarth(p, center) <= kMaxDistanceMeters)
        {
          good = true;
          break;
        }
      }

      if (!good)
        return;

      features.push_back(ft);
    },
    rect, scales::GetUpperScale());

  return features;
}

routing::IRoadGraph::TEdgeVector GetOutgoingEdges(routing::FeaturesRoadGraph const & roadGraph,
                                                  ms::LatLon const & latLon)
{
  routing::IRoadGraph::TEdgeVector edges;

  routing::Junction junction(MercatorBounds::FromLatLon(latLon), feature::kDefaultAltitudeMeters);
  roadGraph.GetOutgoingEdges(junction, edges);

  return edges;
}

routing::IRoadGraph::TEdgeVector GetIngoingEdges(routing::FeaturesRoadGraph const & roadGraph,
                                                 ms::LatLon const & latLon)
{
  routing::IRoadGraph::TEdgeVector edges;

  routing::Junction junction(MercatorBounds::FromLatLon(latLon), feature::kDefaultAltitudeMeters);
  roadGraph.GetIngoingEdges(junction, edges);

  return edges;
}

bool CalculateRoute(routing::IRouter & router, ms::LatLon const & first,
                    ms::LatLon const & last, routing::Route & route)
{
  routing::RouterDelegate delegate;
  auto const result =
      router.CalculateRoute(MercatorBounds::FromLatLon(first), {0.0, 0.0},
                            MercatorBounds::FromLatLon(last), delegate, route);
  if (result != routing::IRouter::ResultCode::NoError)
  {
    LOG(LDEBUG, ("Can't calculate route for points", first, last, ", code:", result));
    return false;
  }
  return true;
}

void LeaveEdgesStartedFrom(routing::Junction const & junction, routing::IRoadGraph::TEdgeVector & edges)
{
  auto count = 0;
  auto const it = remove_if(begin(edges), end(edges), [&junction, &count](routing::Edge const & e)
  {
    bool const eq = !AlmostEqualAbs(e.GetStartJunction(), junction);
    LOG(LDEBUG, (e.GetStartJunction().GetPoint(), "!=", junction.GetPoint(), "->", eq));
    count += eq;
    return eq;
  });
  LOG(LDEBUG, (count, "values should be removed from vector of legth", edges.size()));

  if (it != end(edges))
    edges.erase(it, end(edges));

  LOG(LDEBUG, ("edges current size", edges.size()));
}

routing::Junction JunctionFromPoint(m2::PointD const & p)
{
  return {p, feature::kDefaultAltitudeMeters};
}

struct Stats
{
  uint32_t m_shortRoutes = 0;
  uint32_t m_zeroCanditates = 0;
  uint32_t m_moreThanOneCandidates = 0;
  uint32_t m_routeIsNotCalculated = 0;
  uint32_t m_total = 0;
};

routing::IRoadGraph::TEdgeVector ReconstructPath(routing::IRoadGraph const & graph,
                                                 routing::Route & route, Stats & stats)
{
  routing::IRoadGraph::TEdgeVector path;

  auto poly = route.GetPoly().GetPoints();
  // There are zero-length linear features, so poly can contain adjucent duplications.
  poly.erase(unique(begin(poly), end(poly), [](m2::PointD const & a, m2::PointD const & b)
  {
    return my::AlmostEqualAbs(a, b, routing::kPointsEqualEpsilon);
  }), end(poly));

  // TODO(mgsergio): A rote may strart/end at poits other than edge ends, this situation
  // shoud be handled separately.
  if (poly.size() < 4)
  {
    ++stats.m_shortRoutes;
    LOG(LINFO, ("Short polylines are not handled yet."));
    return {};
  }

  routing::IRoadGraph::TEdgeVector edges;
  // Start from the second point of the route that is the start of the egge for shure.
  auto it = next(begin(poly));
  auto prevJunction = JunctionFromPoint(*it++);

  // Stop at the last point that is the end of the edge for shure.
  for (; it != prev(end(poly)); prevJunction = JunctionFromPoint(*it++))
  {
    // TODO(mgsergio): Check edges are not fake;
    graph.GetIngoingEdges(JunctionFromPoint(*it), edges);
    LOG(LDEBUG, ("Edges extracted:", edges.size()));
    LeaveEdgesStartedFrom(prevJunction, edges);
    if (edges.size() > 1)
    {
      ++stats.m_moreThanOneCandidates;
      LOG(LDEBUG, ("More than one edge candidate."));
    }
    else if (edges.size() == 0)
    {
      ++stats.m_zeroCanditates;
      LOG(LDEBUG, ("Zero edge candidate extracted."));
      // Sometimes a feature may be duplicated in two or more mwms: MAPSME-2816.
      // ASSERT_GREATER_OR_EQUAL(edges.size(), 1,
      //                         ("There should be at least one adge. (One in normal case)"));
      return {};
    }
    path.push_back(edges.front());
    edges.clear();
  }

  return path;
}
}  // namespace

namespace openlr
{
OpenLRSimpleDecoder::OpenLRSimpleDecoder(string const & dataFilename, Index const & index,
                                         routing::IRouter & router)
  : m_index(index)
  , m_router(router)
{
  auto const load_result = m_document.load_file(dataFilename.data());
  if (!load_result)
    MYTHROW(DecoderError, ("Can't load file", dataFilename, ":", load_result.description()));
}

void OpenLRSimpleDecoder::Decode()
{
  routing::FeaturesRoadGraph roadGraph(m_index, routing::IRoadGraph::Mode::ObeyOnewayTag,
                                       make_unique<routing::CarModelFactory>());
  Stats stats;

  ofstream sample("inrix_vs_mwm.txt");

  // TODO(mgsergio): Feed segments derectly to the decoder. Parsing sholud not
  // take place inside decoder process.
  vector<LinearSegment> segments;
  if (!ParseOpenlr(m_document, segments))
    MYTHROW(DecoderError, ("Can't parse data."));

  for (auto const & segment : segments)
  {
    ++stats.m_total;

    // auto const outgoingEdges = GetOutgoingEdges(roadGraph, first);
    // auto const ingoingEdges = GetIngoingEdges(roadGraph, last);

    // LOG(LINFO, ("Number of outgoing enges:", outgoingEdges.size(), "for point: ", first));
    // LOG(LINFO, ("Number of ingoing enges:", ingoingEdges.size(), "for point: ", last));

    // auto const featuresForFirst = GetRoadFeaturesAtPoint(m_index, roadGraph, first);
    // auto const featuresForLast = GetRoadFeaturesAtPoint(m_index, roadGraph, last);

    // cout << "Features: " << featuresForFirst.size() << "\tfor point" << DebugPrint(first) << endl;
    // cout << "Features: " << featuresForLast.size() << "\tfor point" << DebugPrint(last) << endl;

    // LOG(LINFO, ("Number of features", featuresForFirst.size(), "for first point: ", first));
    // LOG(LINFO, ("Number of features", featuresForLast.size(), "for last point: ", last));

    auto const firstPoint = segment.m_locationReference.m_points.front().m_latLon;
    auto const lastPoint = segment.m_locationReference.m_points.back().m_latLon;
    auto const segmentID = segment.m_segmentId;
    LOG(LDEBUG, ("Calculating route from ", firstPoint, lastPoint, "for segment:", segmentID));
    routing::Route route("openlr");
    if (!CalculateRoute(m_router, firstPoint, lastPoint, route))
    {
      ++stats.m_routeIsNotCalculated;
      continue;
    }

    auto const path = ReconstructPath(roadGraph, route, stats);
    LOG(LINFO, ("Route point count:", route.GetPoly().GetSize(), "path legth: ", path.size()));

    if (path.size() == 0)
      continue;

    sample << segmentID << '\t';
    for (auto it = begin(path); it != end(path); ++it)
    {
      auto const & fid = it->GetFeatureId();
      sample << fid.m_mwmId.GetInfo()->GetCountryName() << '-'
             << fid.m_index << '-' << it->GetSegId();
      if (next(it) != end(path))
        sample << '=';
    }
    sample << endl;
  }

  LOG(LINFO, ("Parsed inrix regments:", stats.m_total,
              "Routes failed:", stats.m_routeIsNotCalculated,
              "Short routes:", stats.m_shortRoutes,
              "Ambiguous routes:", stats.m_moreThanOneCandidates,
              "Path is not reconstructed:", stats.m_zeroCanditates));
}
}  // namespace openlr
