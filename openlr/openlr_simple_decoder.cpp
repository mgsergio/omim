#include "openlr/openlr_simple_decoder.hpp"
#include "openlr/openlr_model.hpp"
#include "openlr/openlr_simple_parser.hpp"

#include "routing/car_model.hpp"
#include "routing/car_router.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/road_graph.hpp"
#include "routing/router_delegate.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/scales.hpp"

#include "geometry/angles.hpp"
#include "geometry/latlon.hpp"
#include "geometry/polyline2d.hpp"

#include "platform/location.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"
#include "std/fstream.hpp"
#include "std/map.hpp"
#include "std/queue.hpp"
#include "std/thread.hpp"
#include "std/utility.hpp"

using namespace std::rel_ops;
using namespace routing;

namespace
{
size_t constexpr kCacheLineSize = 64;

size_t const kMaxRoadCandidates = 10;
double const kDistanceAccuracyM = 1000;
double const kEps = 1e-9;
double const kBearingDist = 25;
double const kAnglesInBucket = 360.0 / 256;

size_t constexpr GCD(size_t a, size_t b) { return b == 0 ? a : GCD(b, a % b); }

size_t constexpr LCM(size_t a, size_t b) { return a / GCD(a, b) * b; }

uint32_t Bearing(m2::PointD const & a, m2::PointD const & b)
{
  auto const angle = location::AngleToBearing(my::RadToDeg(ang::AngleTo(a, b)));
  CHECK_LESS_OR_EQUAL(angle, 360, ("Angle should be less than or equal to 360."));
  CHECK_GREATER_OR_EQUAL(angle, 0, ("Angle should be greater than or equal to 0"));
  return my::clamp(angle / kAnglesInBucket, 0, 255);
}

class TrunkChecker : public ftypes::BaseChecker
{
public:
  TrunkChecker()
  {
    auto const & c = classif();
    m_types.push_back(c.GetTypeByPath({"highway", "motorway"}));
    m_types.push_back(c.GetTypeByPath({"highway", "motorway_link"}));
    m_types.push_back(c.GetTypeByPath({"highway", "trunk"}));
    m_types.push_back(c.GetTypeByPath({"highway", "trunk_link"}));
  }
};

class PrimaryChecker : public ftypes::BaseChecker
{
public:
  PrimaryChecker()
  {
    auto const & c = classif();
    m_types.push_back(c.GetTypeByPath({"highway", "primary"}));
    m_types.push_back(c.GetTypeByPath({"highway", "primary_link"}));
  }
};

class SecondaryChecker : public ftypes::BaseChecker
{
public:
  SecondaryChecker()
  {
    auto const & c = classif();

    m_types.push_back(c.GetTypeByPath({"highway", "secondary"}));
    m_types.push_back(c.GetTypeByPath({"highway", "secondary_link"}));
  }
};

class TertiaryChecker : public ftypes::BaseChecker
{
public:
  TertiaryChecker()
  {
    auto const & c = classif();
    m_types.push_back(c.GetTypeByPath({"highway", "tertiary"}));
    m_types.push_back(c.GetTypeByPath({"highway", "tertiary_link"}));
  }
};

class ResidentialChecker : public ftypes::BaseChecker
{
 public:
  ResidentialChecker()
  {
    auto const & c = classif();
    m_types.push_back(c.GetTypeByPath({"highway", "road"}));
    m_types.push_back(c.GetTypeByPath({"highway", "unclassified"}));
    m_types.push_back(c.GetTypeByPath({"highway", "residential"}));
  }
};

class LivingStreetChecker : public ftypes::BaseChecker
{
 public:
  LivingStreetChecker()
  {
    auto const & c = classif();
    m_types.push_back(c.GetTypeByPath({"highway", "living_street"}));
  }
};

class RoadInfoGetter
{
public:
  struct RoadInfo
  {
    openlr::FunctionalRoadClass m_frc;
    openlr::FormOfAWay m_fow;
  };

  RoadInfoGetter(Index const & index): m_index(index), m_c(classif()) {}

  /// Returns true if the |edge|'s road class is more important
  /// (stands higher in openlr::FunctionalRoadClass definition) than
  /// the restriction or equals to it.  Ex: FRC0 denotes a road with a
  /// higher importance than FRC1.
  bool PassFRCLowesRestriction(Edge const & edge, openlr::FunctionalRoadClass const restriction)
  {
    if (edge.IsFake())
      return true;

    auto const frc = GetFeatureRoadInfo(edge.GetFeatureId()).m_frc;
    return frc <= restriction;
  }

private:
  RoadInfo GetFeatureRoadInfo(FeatureID const & fid)
  {
    auto it = m_cache.find(fid);
    if (it == end(m_cache))
    {
      Index::FeaturesLoaderGuard g(m_index, fid.m_mwmId);
      FeatureType ft;
      CHECK(g.GetOriginalFeatureByIndex(fid.m_index, ft), ());
      ft.ParseTypes();
      RoadInfo info;
      info.m_frc = GetFunctionalRoadClass(ft);
      info.m_fow = GetFormOfAWay(ft);
      it = m_cache.emplace(fid, info).first;
    }

    return it->second;
  }

  openlr::FunctionalRoadClass GetFunctionalRoadClass(feature::TypesHolder const & types) const
  {
    if (m_trunkChecker(types))
      return openlr::FunctionalRoadClass::FRC0;

    if (m_primaryChecker(types))
      return openlr::FunctionalRoadClass::FRC1;

    if (m_secondaryChecker(types))
      return openlr::FunctionalRoadClass::FRC2;

    if (m_tertiaryChecker(types))
      return openlr::FunctionalRoadClass::FRC3;

    if (m_residentialChecker(types))
      return openlr::FunctionalRoadClass::FRC4;

    if (m_livingStreetChecker(types))
      return openlr::FunctionalRoadClass::FRC5;

    return openlr::FunctionalRoadClass::FRC7;
  }

  openlr::FormOfAWay GetFormOfAWay(feature::TypesHolder const & types) const
  {
    if (m_trunkChecker(types))
      return openlr::FormOfAWay::Motorway;

    if (m_primaryChecker(types))
      return openlr::FormOfAWay::MultipleCarriageway;

    return openlr::FormOfAWay::SingleCarriageway;
  }

  Index const & m_index;
  Classificator const & m_c;

  TrunkChecker const m_trunkChecker;
  PrimaryChecker const m_primaryChecker;
  SecondaryChecker const m_secondaryChecker;
  TertiaryChecker const m_tertiaryChecker;
  ResidentialChecker const m_residentialChecker;
  LivingStreetChecker const m_livingStreetChecker;

  map<FeatureID, RoadInfo> m_cache;
};

struct InrixPoint
{
  InrixPoint() = default;

  InrixPoint(openlr::LocationReferencePoint const & lrp)
    : m_point(MercatorBounds::FromLatLon(lrp.m_latLon))
    , m_distanceToNextPointM(lrp.m_distanceToNextPoint)
    , m_bearing(lrp.m_bearing)
    , m_lfrcnp(lrp.m_functionalRoadClass)
  {
  }

  m2::PointD m_point = m2::PointD::Zero();
  double m_distanceToNextPointM = 0.0;
  uint8_t m_bearing = 0;
  openlr::FunctionalRoadClass m_lfrcnp = openlr::FunctionalRoadClass::NotAValue;
};

class AStarRouter
{
public:
  AStarRouter(FeaturesRoadGraph & graph, RoadInfoGetter & roadInfoGetter)
    : m_graph(graph)
    , m_roadInfoGetter(roadInfoGetter)
  {
  }

  bool Go(vector<InrixPoint> const & points, vector<Edge> & path)
  {
    CHECK_GREATER_OR_EQUAL(points.size(), 2, ());

    m_graph.ResetFakes();

    m_pivots.clear();
    for (size_t i = 1; i + 1 < points.size(); ++i)
    {
      m_pivots.emplace_back();
      auto & ps = m_pivots.back();

      vector<pair<Edge, Junction>> vicinity;
      m_graph.FindClosestEdges(points[i].m_point, kMaxRoadCandidates, vicinity);
      for (auto const & v : vicinity)
      {
        auto const & e = v.first;
        ps.push_back(e.GetStartJunction().GetPoint());
        ps.push_back(e.GetEndJunction().GetPoint());
      }
    }
    m_pivots.push_back({points.back().m_point});
    CHECK_EQUAL(m_pivots.size() + 1, points.size(), ());

    Junction js(points.front().m_point, 0 /* altitude */);

    {
      vector<pair<Edge, Junction>> sourceVicinity;
      m_graph.FindClosestEdges(points.front().m_point, kMaxRoadCandidates, sourceVicinity);
      m_graph.AddFakeEdges(js, sourceVicinity);
    }

    Junction jt(points.back().m_point, 0 /* altitude */);
    {
      vector<pair<Edge, Junction>> targetVicinity;
      m_graph.FindClosestEdges(points.back().m_point, kMaxRoadCandidates, targetVicinity);
      m_graph.AddFakeEdges(jt, targetVicinity);
    }

    using State = pair<Score, Vertex>;
    priority_queue<State, vector<State>, greater<State>> queue;
    map<Vertex, Score> scores;
    Links links;

    auto pushVertex = [&queue, &scores, &links](Vertex const & u, Vertex const & v,
                                                Score const & sv, Edge const & e) {
      if ((scores.count(v) == 0 || scores[v] > sv) && u != v)
      {
        scores[v] = sv;
        links[v] = make_pair(u, e);
        queue.emplace(sv, v);
      }
    };

    Vertex const s(js, js, 0 /* stageStartDistance */, 0 /* stage */, false /* bearingChecked */);
    scores[s] = Score();
    queue.emplace(scores[s], s);

    double const piS = GetPotential(s);

    while (!queue.empty())
    {
      auto const p = queue.top();
      queue.pop();

      Score const & su = p.first;
      Vertex const & u = p.second;

      if (su != scores[u])
        continue;

      if (u.m_stage == m_pivots.size())
      {
        auto cur = u;
        while (cur != s)
        {
          auto const & p = links[cur];
          path.push_back(p.second);
          cur = p.first;
        }
        reverse(path.begin(), path.end());
        return true;
      }

      size_t const stage = u.m_stage;
      double const distanceToNextPointM = points[stage].m_distanceToNextPointM;
      double const piU = GetPotential(u);
      double const ud = su.GetDistance() + piS - piU;  // real distance to u

      CHECK_LESS(stage, m_pivots.size(), ());

      // max(kDistanceAccuracyM, m_distanceToNextPointM) is added here
      // to throw out quite long paths.
      if (ud > u.m_stageStartDistance + distanceToNextPointM +
          max(kDistanceAccuracyM, distanceToNextPointM))
      {
        continue;
      }

      if (piU < kEps && !u.m_bearingChecked)
      {
        Vertex v = u;
        v.m_bearingChecked = true;

        Score sv = su;
        if (u.m_junction != u.m_stageStart)
        {
          int const expected = points[stage].m_bearing;
          int const actual = Bearing(u.m_stageStart.GetPoint(), u.m_junction.GetPoint());
          sv.AddBearingPenalty(expected, actual);
        }

        pushVertex(u, v, sv, Edge::MakeFake(u.m_junction, v.m_junction));
      }

      if (piU < kEps && u.m_bearingChecked)
      {
        Vertex v(u.m_junction, u.m_junction, ud /* stageStartDistance */, stage + 1, false /* bearingChecked */);
        bool const isLastVertex = stage + 1 == m_pivots.size();

        double const piV = isLastVertex ? 0 : GetPotential(v);

        Score sv = su;
        sv.AddDistance(max(piV - piU, 0.0));
        sv.AddIntermediateErrorPenalty(
            MercatorBounds::DistanceOnEarth(u.m_junction.GetPoint(), points[stage + 1].m_point));

        if (isLastVertex)
        {
          int const expected = points.back().m_bearing;
          int const actual = GetReverseBearing(u, links);
          sv.AddBearingPenalty(expected, actual);
        }

        pushVertex(u, v, sv, Edge::MakeFake(u.m_junction, v.m_junction));

        if (isLastVertex)
          continue;
      }

      IRoadGraph::TEdgeVector edges;
      m_graph.GetOutgoingEdges(u.m_junction, edges);
      for (auto const & edge : edges)
      {
        if (!m_roadInfoGetter.PassFRCLowesRestriction(edge, points[stage].m_lfrcnp))
          continue;

        Vertex v(edge.GetEndJunction(), u.m_stageStart, u.m_stageStartDistance, stage,
                 u.m_bearingChecked);
        double const piV = GetPotential(v);

        Score sv = su;
        double const w = GetWeight(edge);
        sv.AddDistance(max(w + piV - piU, 0.0));

        double const vd = ud + w;  // real distance to v
        if (!v.m_bearingChecked && vd >= u.m_stageStartDistance + kBearingDist)
        {
          ASSERT_LESS(ud, u.m_stageStartDistance + kBearingDist, ());
          double const delta = vd - u.m_stageStartDistance - kBearingDist;
          auto const p = PointAtSegment(edge.GetStartJunction().GetPoint(),
                                        edge.GetEndJunction().GetPoint(), delta);
          if (u.m_stageStart.GetPoint() != p)
          {
            int const expected = points[stage].m_bearing;
            int const actual = Bearing(u.m_stageStart.GetPoint(), p);
            sv.AddBearingPenalty(expected, actual);
          }
          v.m_bearingChecked = true;
        }

        if (vd > v.m_stageStartDistance + distanceToNextPointM)
        {
          sv.AddDistanceErrorPenalty(
              std::min(vd - v.m_stageStartDistance - distanceToNextPointM, w));
        }

        if (edge.IsFake())
          sv.AddFakePenalty(w);

        pushVertex(u, v, sv, edge);
      }
    }

    return false;
  }

private:
  struct Vertex
  {
    Vertex() = default;
    Vertex(Junction const & junction, Junction const & stageStart, double stageStartDistance,
           size_t stage, bool bearingChecked)
      : m_junction(junction)
      , m_stageStart(stageStart)
      , m_stageStartDistance(stageStartDistance)
      , m_stage(stage)
      , m_bearingChecked(bearingChecked)
    {
    }

    inline bool operator<(Vertex const & rhs) const
    {
      if (m_stage != rhs.m_stage)
        return m_stage < rhs.m_stage;
      if (m_junction != rhs.m_junction)
        return m_junction < rhs.m_junction;
      if (m_stageStart != rhs.m_stageStart)
        return m_stageStart < rhs.m_stageStart;
      return m_bearingChecked < rhs.m_bearingChecked;
    }

    inline bool operator==(Vertex const & rhs) const
    {
      return m_junction == rhs.m_junction && m_stageStart == rhs.m_stageStart &&
             m_stage == rhs.m_stage && m_bearingChecked == rhs.m_bearingChecked;
    }

    Junction m_junction;
    Junction m_stageStart;
    double m_stageStartDistance = 0.0;
    size_t m_stage = 0;
    bool m_bearingChecked = false;
  };

  using Links = map<Vertex, pair<Vertex, Edge>>;

  class Score
  {
  public:
    static const int kFakeCoeff = 3;
    static const int kIntermediateErrorCoeff = 3;
    static const int kDistanceErrorCoeff = 3;
    static const int kBearingErrorCoeff = 5;

    inline double GetDistance() const { return m_distance; }

    inline void AddDistance(double p)
    {
      m_distance += p;
      Update();
    }

    inline void AddFakePenalty(double p)
    {
      m_fakePenalty += p;
      Update();
    }

    inline void AddIntermediateErrorPenalty(double p)
    {
      m_intermediateErrorPenalty += p;
      Update();
    }

    inline void AddDistanceErrorPenalty(double p)
    {
      m_distanceErrorPenalty += p;
      Update();
    }

    inline void AddBearingPenalty(int expected, int actual)
    {
      double angle = my::DegToRad(abs(expected - actual) * kAnglesInBucket);
      m_bearingPenalty += angle * kBearingDist;
    }

    bool operator<(Score const & rhs) const { return m_score < rhs.m_score; }
    bool operator==(Score const & rhs) const { return m_score == rhs.m_score; }

  private:
    void Update()
    {
      m_score = m_distance + kFakeCoeff * m_fakePenalty +
                kIntermediateErrorCoeff * m_intermediateErrorPenalty +
                kDistanceErrorCoeff * m_distanceErrorPenalty +
                kBearingErrorCoeff * m_bearingPenalty;
      CHECK_GREATER_OR_EQUAL(m_score, 0, ());
    }

    // Reduced length of path in meters.
    double m_distance = 0.0;

    // Penalty of passing by fake edges.
    double m_fakePenalty = 0.0;

    // Penalty of passing through an candidate that is too far from
    // corresponding intermediate point.
    double m_intermediateErrorPenalty = 0.0;

    // Penalty of path between consecutive points that is longer than
    // required.
    double m_distanceErrorPenalty = 0.0;

    double m_bearingPenalty = 0.0;

    // Total score.
    double m_score = 0.0;
  };

  double GetPotential(Vertex const & u) const
  {
    CHECK_LESS(u.m_stage, m_pivots.size(), ());

    auto const & pivots = m_pivots[u.m_stage];

    double potential = numeric_limits<double>::max();

    auto const & point = u.m_junction.GetPoint();
    for (auto const & pivot : pivots)
      potential = min(potential, MercatorBounds::DistanceOnEarth(pivot, point));
    return potential;
  }

  double Distance(Junction const & u, Junction const & v) const
  {
    return MercatorBounds::DistanceOnEarth(u.GetPoint(), v.GetPoint());
  }

  double GetWeight(Edge const & e) const
  {
    return Distance(e.GetStartJunction(), e.GetEndJunction());
  }

  uint32_t GetReverseBearing(Vertex const & u, Links const & links) const
  {
    m2::PointD const a = u.m_junction.GetPoint();
    m2::PointD b = m2::PointD::Zero();

    Vertex curr = u;
    double passed = 0;
    bool found = false;
    while (true)
    {
      auto const it = links.find(curr);
      if (it == links.end())
        break;

      auto const & p = it->second;
      auto const & prev = p.first;
      auto const & edge = p.second;

      if (prev.m_stage != curr.m_stage)
        break;

      double const weight = GetWeight(edge);

      if (passed + weight >= kBearingDist)
      {
        double const delta = kBearingDist - passed;
        b = PointAtSegment(edge.GetEndJunction().GetPoint(), edge.GetStartJunction().GetPoint(),
                           delta);
        found = true;
        break;
      }

      passed += weight;
      curr = prev;
    }
    if (!found)
      b = curr.m_junction.GetPoint();
    return Bearing(a, b);
  }

  FeaturesRoadGraph & m_graph;
  RoadInfoGetter & m_roadInfoGetter;
  vector<vector<m2::PointD>> m_pivots;
};

struct alignas(kCacheLineSize) Stats
{
  void Add(Stats const & rhs)
  {
    m_shortRoutes += rhs.m_shortRoutes;
    m_zeroCanditates += rhs.m_zeroCanditates;
    m_moreThanOneCandidates += rhs.m_moreThanOneCandidates;
    m_routeIsNotCalculated += rhs.m_routeIsNotCalculated;
    m_total += rhs.m_total;
  }

  uint32_t m_shortRoutes = 0;
  uint32_t m_zeroCanditates = 0;
  uint32_t m_moreThanOneCandidates = 0;
  uint32_t m_routeIsNotCalculated = 0;
  uint32_t m_total = 0;
};
}  // namespace

namespace openlr
{
int const OpenLRSimpleDecoder::kHandleAllSegments = -1;

OpenLRSimpleDecoder::OpenLRSimpleDecoder(string const & dataFilename, Index const & index)
  : m_index(index)
{
  auto const load_result = m_document.load_file(dataFilename.data());
  if (!load_result)
    MYTHROW(DecoderError, ("Can't load file", dataFilename, ":", load_result.description()));
}

void OpenLRSimpleDecoder::Decode(string const & outputFilename, int const segmentsToHandle,
                                 bool const multipointsOnly, int const numThreads)
{
  // TODO(mgsergio): Feed segments derectly to the decoder. Parsing sholud not
  // take place inside decoder process.
  vector<LinearSegment> segments;
  if (!ParseOpenlr(m_document, segments))
    MYTHROW(DecoderError, ("Can't parse data."));

  if (multipointsOnly)
  {
    my::EraseIf(segments, [](LinearSegment const & segment) {
      return segment.m_locationReference.m_points.size() == 2;
    });
  }

  if (segmentsToHandle != kHandleAllSegments && segmentsToHandle < segments.size())
    segments.resize(segmentsToHandle);

  sort(segments.begin(), segments.end(), my::LessBy(&LinearSegment::m_segmentId));

  vector<IRoadGraph::TEdgeVector> paths(segments.size());

  // This code computes the most optimal (in the sense of cache lines
  // occupancy) batch size.
  size_t constexpr a = LCM(sizeof(LinearSegment), kCacheLineSize) / sizeof(LinearSegment);
  size_t constexpr b =
      LCM(sizeof(IRoadGraph::TEdgeVector), kCacheLineSize) / sizeof(IRoadGraph::TEdgeVector);
  size_t constexpr kBatchSize = LCM(a, b);
  size_t constexpr kProgressFrequency = 100;

  auto worker = [&segments, &paths, kBatchSize, kProgressFrequency, numThreads, this](
      size_t threadNum, Stats & stats) {
    FeaturesRoadGraph roadGraph(m_index, IRoadGraph::Mode::ObeyOnewayTag,
                                make_unique<CarModelFactory>());
    RoadInfoGetter roadInfoGetter(m_index);
    AStarRouter astarRouter(roadGraph, roadInfoGetter);

    size_t const numSegments = segments.size();

    vector<InrixPoint> points;

    for (size_t i = threadNum * kBatchSize; i < numSegments; i += numThreads * kBatchSize)
    {
      for (size_t j = i; j < numSegments && j < i + kBatchSize; ++j)
      {
        auto const & segment = segments[j];

        points.clear();
        for (auto const & point : segment.m_locationReference.m_points)
          points.emplace_back(point);

        auto & path = paths[j];
        if (astarRouter.Go(points, path))
        {
          path.erase(
              remove_if(path.begin(), path.end(), [](Edge const & edge) { return edge.IsFake(); }),
              path.end());
        }
        else
        {
          ++stats.m_routeIsNotCalculated;
        }

        ++stats.m_total;

        if (stats.m_total % kProgressFrequency == 0)
          LOG(LINFO, ("Thread", threadNum, "processed:", stats.m_total));
      }
    }
  };

  vector<Stats> stats(numThreads);
  vector<thread> workers;
  for (size_t i = 1; i < numThreads; ++i)
    workers.emplace_back(worker, i, ref(stats[i]));
  worker(0 /* threadNum */, stats[0]);
  for (auto & worker : workers)
    worker.join();

  ofstream sample(outputFilename);
  for (size_t i = 0; i < segments.size(); ++i)
  {
    auto const & segment = segments[i];
    auto const & path = paths[i];

    if (path.empty())
      continue;

    sample << segment.m_segmentId << '\t';
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

  Stats allStats;
  for (auto const & s : stats)
    allStats.Add(s);

  LOG(LINFO, ("Parsed segments:", allStats.m_total,
              "Routes failed:", allStats.m_routeIsNotCalculated,
              "Short routes:", allStats.m_shortRoutes,
              "Ambiguous routes:", allStats.m_moreThanOneCandidates,
              "Path is not reconstructed:", allStats.m_zeroCanditates));
}
}  // namespace openlr
