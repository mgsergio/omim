#include "testing/testing.hpp"

#include "routing/route.hpp"
#include "routing/turns.hpp"
#include "routing/turns_generator.hpp"

#include "indexer/mercator.hpp"

#include "geometry/point2d.hpp"

#include "std/cmath.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

using namespace routing;
using namespace turns;

namespace
{
UNIT_TEST(TestSplitLanes)
{
  vector<string> result;
  SplitLanes("through|through|through|through;right", '|', result);
  vector<string> expected1 = {"through", "through", "through", "through;right"};
  TEST_EQUAL(result, expected1, ());

  SplitLanes("adsjkddfasui8747&sxdsdlad8\"\'", '|', result);
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0], "adsjkddfasui8747&sxdsdlad8\"\'", ());

  SplitLanes("|||||||", '|', result);
  vector<string> expected2 = {"", "", "", "", "", "", ""};
  TEST_EQUAL(result, expected2, ());
}

UNIT_TEST(TestParseSingleLane)
{
  TSingleLane result;
  TEST(ParseSingleLane("through;right", ';', result), ());
  TSingleLane expected1 = {LaneWay::Through, LaneWay::Right};
  TEST_EQUAL(result, expected1, ());

  TEST(!ParseSingleLane("through;Right", ';', result), ());

  TEST(!ParseSingleLane("through ;right", ';', result), ());
  TEST_EQUAL(result.size(), 0, ());

  TEST(!ParseSingleLane("SD32kk*887;;", ';', result), ());
  TEST_EQUAL(result.size(), 0, ());

  TEST(!ParseSingleLane("Что-то на кириллице", ';', result), ());
  TEST_EQUAL(result.size(), 0, ());

  TEST(!ParseSingleLane("משהו בעברית", ';', result), ());
  TEST_EQUAL(result.size(), 0, ());

  TEST(ParseSingleLane("left;through", ';', result), ());
  TSingleLane expected2 = {LaneWay::Left, LaneWay::Through};
  TEST_EQUAL(result, expected2, ());

  TEST(ParseSingleLane("left", ';', result), ());
  TEST_EQUAL(result.size(), 1, ());
  TEST_EQUAL(result[0], LaneWay::Left, ());
}

UNIT_TEST(TestParseLanes)
{
  vector<SingleLaneInfo> result;
  TEST(ParseLanes("through|through|through|through;right", result), ());
  TEST_EQUAL(result.size(), 4, ());
  TEST_EQUAL(result[0].m_lane.size(), 1, ());
  TEST_EQUAL(result[3].m_lane.size(), 2, ());
  TEST_EQUAL(result[0].m_lane[0], LaneWay::Through, ());
  TEST_EQUAL(result[3].m_lane[0], LaneWay::Through, ());
  TEST_EQUAL(result[3].m_lane[1], LaneWay::Right, ());

  TEST(ParseLanes("left|left;through|through|through", result), ());
  TEST_EQUAL(result.size(), 4, ());
  TEST_EQUAL(result[0].m_lane.size(), 1, ());
  TEST_EQUAL(result[1].m_lane.size(), 2, ());
  TEST_EQUAL(result[3].m_lane.size(), 1, ());
  TEST_EQUAL(result[0].m_lane[0], LaneWay::Left, ());
  TEST_EQUAL(result[1].m_lane[0], LaneWay::Left, ());
  TEST_EQUAL(result[1].m_lane[1], LaneWay::Through, ());
  TEST_EQUAL(result[3].m_lane[0], LaneWay::Through, ());

  TEST(ParseLanes("left|through|through", result), ());
  TEST_EQUAL(result.size(), 3, ());
  TEST_EQUAL(result[0].m_lane.size(), 1, ());
  TEST_EQUAL(result[1].m_lane.size(), 1, ());
  TEST_EQUAL(result[2].m_lane.size(), 1, ());
  TEST_EQUAL(result[0].m_lane[0], LaneWay::Left, ());
  TEST_EQUAL(result[1].m_lane[0], LaneWay::Through, ());
  TEST_EQUAL(result[2].m_lane[0], LaneWay::Through, ());

  TEST(ParseLanes("left|le  ft|   through|through   |  right", result), ());
  TEST_EQUAL(result.size(), 5, ());
  TEST_EQUAL(result[0].m_lane.size(), 1, ());
  TEST_EQUAL(result[4].m_lane.size(), 1, ());
  TEST_EQUAL(result[0].m_lane[0], LaneWay::Left, ());
  TEST_EQUAL(result[1].m_lane[0], LaneWay::Left, ());
  TEST_EQUAL(result[2].m_lane[0], LaneWay::Through, ());
  TEST_EQUAL(result[3].m_lane[0], LaneWay::Through, ());
  TEST_EQUAL(result[4].m_lane[0], LaneWay::Right, ());

  TEST(ParseLanes("left|Left|through|througH|right", result), ());
  TEST_EQUAL(result.size(), 5, ());
  TEST_EQUAL(result[0].m_lane.size(), 1, ());
  TEST_EQUAL(result[4].m_lane.size(), 1, ());
  TEST_EQUAL(result[0].m_lane[0], LaneWay::Left, ());
  TEST_EQUAL(result[4].m_lane[0], LaneWay::Right, ());

  TEST(ParseLanes("left|Left|through|througH|through;right;sharp_rIght", result), ());
  TEST_EQUAL(result.size(), 5, ());
  TEST_EQUAL(result[0].m_lane.size(), 1, ());
  TEST_EQUAL(result[4].m_lane.size(), 3, ());
  TEST_EQUAL(result[0].m_lane[0], LaneWay::Left, ());
  TEST_EQUAL(result[4].m_lane[0], LaneWay::Through, ());
  TEST_EQUAL(result[4].m_lane[1], LaneWay::Right, ());
  TEST_EQUAL(result[4].m_lane[2], LaneWay::SharpRight, ());

  TEST(!ParseLanes("left|Leftt|through|througH|right", result), ());
  TEST_EQUAL(result.size(), 0, ());

  TEST(!ParseLanes("Что-то на кириллице", result), ());
  TEST_EQUAL(result.size(), 0, ());

  TEST(!ParseLanes("משהו בעברית", result), ());
  TEST_EQUAL(result.size(), 0, ());

  TEST(ParseLanes("left |Left|through|througH|right", result), ());
  TEST_EQUAL(result.size(), 5, ());
  TEST_EQUAL(result[0].m_lane[0], LaneWay::Left, ());
  TEST_EQUAL(result[1].m_lane[0], LaneWay::Left, ());
}

UNIT_TEST(TestFixupTurns)
{
  double const kHalfSquareSideMeters = 10.;
  m2::PointD const kSquareCenterLonLat = {0., 0.};
  m2::RectD const kSquareNearZero = MercatorBounds::MetresToXY(kSquareCenterLonLat.x,
                                                               kSquareCenterLonLat.y, kHalfSquareSideMeters);
  // Removing a turn in case staying on a roundabout.
  vector<m2::PointD> const pointsMerc1 = {
    { kSquareNearZero.minX(), kSquareNearZero.minY()},
    { kSquareNearZero.minX(), kSquareNearZero.maxY() },
    { kSquareNearZero.maxX(), kSquareNearZero.maxY() },
    { kSquareNearZero.maxX(), kSquareNearZero.minY() }
  };
  // The constructor TurnItem(uint32_t idx, TurnDirection t, uint32_t exitNum = 0)
  // is used for initialization of vector<TurnItem> below.
  Route::TurnsT turnsDir1 = {
    { 0, TurnDirection::EnterRoundAbout },
    { 1, TurnDirection::StayOnRoundAbout },
    { 2, TurnDirection::LeaveRoundAbout },
    { 3, TurnDirection::ReachedYourDestination }
  };

  FixupTurns(pointsMerc1, turnsDir1);
  Route::TurnsT expectedTurnDir1 = {
    { 0, TurnDirection::EnterRoundAbout, 2 },
    { 2, TurnDirection::LeaveRoundAbout },
    { 3, TurnDirection::ReachedYourDestination }
  };
  TEST_EQUAL(turnsDir1, expectedTurnDir1, ());

  // Merging turns which are close to each other.
  vector<m2::PointD> const pointsMerc2 = {
    { kSquareNearZero.minX(), kSquareNearZero.minY()},
    { kSquareCenterLonLat.x, kSquareCenterLonLat.y },
    { kSquareNearZero.maxX(), kSquareNearZero.maxY() }};
  Route::TurnsT turnsDir2 = {
    { 0, TurnDirection::GoStraight },
    { 1, TurnDirection::TurnLeft },
    { 2, TurnDirection::ReachedYourDestination }
  };

  FixupTurns(pointsMerc2, turnsDir2);
  Route::TurnsT expectedTurnDir2 = {
    { 1, TurnDirection::TurnLeft },
    { 2, TurnDirection::ReachedYourDestination }
  };
  TEST_EQUAL(turnsDir2, expectedTurnDir2, ());

  // No turn is removed.
  vector<m2::PointD> const pointsMerc3 = {
    { kSquareNearZero.minX(), kSquareNearZero.minY()},
    { kSquareNearZero.minX(), kSquareNearZero.maxY() },
    { kSquareNearZero.maxX(), kSquareNearZero.maxY() }};
  Route::TurnsT turnsDir3 = {
    { 1, TurnDirection::TurnRight },
    { 2, TurnDirection::ReachedYourDestination }
  };

  FixupTurns(pointsMerc3, turnsDir3);
  Route::TurnsT expectedTurnDir3 = {
    { 1, TurnDirection::TurnRight },
    { 2, TurnDirection::ReachedYourDestination }
  };
  TEST_EQUAL(turnsDir3, expectedTurnDir3, ());
}

UNIT_TEST(TestCalculateTurnGeometry)
{
  double constexpr kHalfSquareSideMeters = 10.;
  double constexpr kSquareSideMeters = 2 * kHalfSquareSideMeters;
  double const kErrorMeters = 1.;
  m2::PointD const kSquareCenterLonLat = {0., 0.};
  m2::RectD const kSquareNearZero = MercatorBounds::MetresToXY(kSquareCenterLonLat.x,
                                                               kSquareCenterLonLat.y, kHalfSquareSideMeters);

  // Empty vectors
  vector<m2::PointD> const points1;
  Route::TurnsT const turnsDir1;
  TurnsGeomT turnsGeom1;
  CalculateTurnGeometry(points1, turnsDir1, turnsGeom1);
  TEST(turnsGeom1.empty(), ());

  // A turn is in the middle of a very short route.
  vector<m2::PointD> const points2 = {
    { kSquareNearZero.minX(), kSquareNearZero.minY() },
    { kSquareNearZero.maxX(), kSquareNearZero.maxY() },
    { kSquareNearZero.minX(), kSquareNearZero.maxY() }
  };
  Route::TurnsT const turnsDir2 = {
    { 1, TurnDirection::TurnLeft },
    { 2, TurnDirection::ReachedYourDestination }
  };
  TurnsGeomT turnsGeom2;

  CalculateTurnGeometry(points2, turnsDir2, turnsGeom2);
  TEST_EQUAL(turnsGeom2.size(), 1, ());
  TEST_EQUAL(turnsGeom2[0].m_indexInRoute, 1, ());
  TEST_EQUAL(turnsGeom2[0].m_turnIndex, 1, ());
  TEST_EQUAL(turnsGeom2[0].m_points.size(), 3, ());
  TEST_LESS(fabs(MercatorBounds::DistanceOnEarth(turnsGeom2[0].m_points[1],
                                                 turnsGeom2[0].m_points[2]) - kSquareSideMeters),
                                                 kErrorMeters, ());

  // Two turns. One is in the very beginnig of a short route and another one is at the end.
  // The first turn is located at point 0 and the second one at the point 3.
  //  1----->2
  //  ^      |
  //  |   4  |
  //  |    \ v
  //  0      3
  vector<m2::PointD> const points3 = {
    { kSquareNearZero.minX(), kSquareNearZero.minY() },
    { kSquareNearZero.minX(), kSquareNearZero.maxY() },
    { kSquareNearZero.maxX(), kSquareNearZero.maxY() },
    { kSquareNearZero.maxX(), kSquareNearZero.minY() },
    { kSquareCenterLonLat.x, kSquareCenterLonLat.y }
  };
  Route::TurnsT const turnsDir3 = {
    { 0, TurnDirection::GoStraight },
    { 3, TurnDirection::TurnSharpRight },
    { 4, TurnDirection::ReachedYourDestination }
  };
  TurnsGeomT turnsGeom3;

  CalculateTurnGeometry(points3, turnsDir3, turnsGeom3);
  TEST_EQUAL(turnsGeom3.size(), 1, ());
  TEST_EQUAL(turnsGeom3[0].m_indexInRoute, 3, ());
  TEST_EQUAL(turnsGeom3[0].m_turnIndex, 3, ());
  TEST_EQUAL(turnsGeom3[0].m_points.size(), 5, ());
  TEST_LESS(fabs(MercatorBounds::DistanceOnEarth(turnsGeom3[0].m_points[1],
                                                 turnsGeom3[0].m_points[2]) - kSquareSideMeters),
                                                 kErrorMeters, ());
  TEST_LESS(fabs(MercatorBounds::DistanceOnEarth(turnsGeom3[0].m_points[2],
                                                 turnsGeom3[0].m_points[3]) - kSquareSideMeters),
                                                 kErrorMeters, ());
}

UNIT_TEST(TestIsLaneWayConformedTurnDirection)
{
  TEST(IsLaneWayConformedTurnDirection(LaneWay::Left, TurnDirection::TurnLeft), ());
  TEST(IsLaneWayConformedTurnDirection(LaneWay::Right, TurnDirection::TurnRight), ());
  TEST(IsLaneWayConformedTurnDirection(LaneWay::SlightLeft, TurnDirection::TurnSlightLeft), ());
  TEST(IsLaneWayConformedTurnDirection(LaneWay::SharpRight, TurnDirection::TurnSharpRight), ());
  TEST(IsLaneWayConformedTurnDirection(LaneWay::Reverse, TurnDirection::UTurn), ());
  TEST(IsLaneWayConformedTurnDirection(LaneWay::Through, TurnDirection::GoStraight), ());

  TEST(!IsLaneWayConformedTurnDirection(LaneWay::Left, TurnDirection::TurnSlightLeft), ());
  TEST(!IsLaneWayConformedTurnDirection(LaneWay::Right, TurnDirection::TurnSharpRight), ());
  TEST(!IsLaneWayConformedTurnDirection(LaneWay::SlightLeft, TurnDirection::GoStraight), ());
  TEST(!IsLaneWayConformedTurnDirection(LaneWay::SharpRight, TurnDirection::NoTurn), ());
  TEST(!IsLaneWayConformedTurnDirection(LaneWay::Reverse, TurnDirection::TurnLeft), ());
  TEST(!IsLaneWayConformedTurnDirection(LaneWay::None, TurnDirection::ReachedYourDestination), ());
}

UNIT_TEST(TestIsLaneWayConformedTurnDirectionApproximately)
{
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Left, TurnDirection::TurnSharpLeft), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Left, TurnDirection::TurnSlightLeft), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Right, TurnDirection::TurnSharpRight), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Right, TurnDirection::TurnRight), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::Reverse, TurnDirection::UTurn), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::SlightLeft, TurnDirection::GoStraight), ());
  TEST(IsLaneWayConformedTurnDirectionApproximately(LaneWay::SlightRight, TurnDirection::GoStraight), ());

  TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::SharpLeft, TurnDirection::UTurn), ());
  TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::SharpRight, TurnDirection::UTurn), ());
  TEST(!IsLaneWayConformedTurnDirection(LaneWay::Through, TurnDirection::ReachedYourDestination), ());
  TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::Through, TurnDirection::TurnRight), ());
  TEST(!IsLaneWayConformedTurnDirectionApproximately(LaneWay::SlightRight, TurnDirection::TurnSharpLeft), ());
}

UNIT_TEST(TestAddingActiveLaneInformation)
{
  Route::TurnsT turns = {{0, TurnDirection::GoStraight},
                         {1, TurnDirection::TurnLeft},
                         {2, TurnDirection::ReachedYourDestination}};
  turns[0].m_lanes.push_back({LaneWay::Left, LaneWay::Through});
  turns[0].m_lanes.push_back({LaneWay::Right});

  turns[1].m_lanes.push_back({LaneWay::SlightLeft});
  turns[1].m_lanes.push_back({LaneWay::Through});
  turns[1].m_lanes.push_back({LaneWay::Through});

  AddingActiveLaneInformation(turns);

  TEST(turns[0].m_lanes[0].m_isActive, ());
  TEST(!turns[0].m_lanes[1].m_isActive, ());

  TEST(turns[1].m_lanes[0].m_isActive, ());
  TEST(!turns[1].m_lanes[1].m_isActive, ());
  TEST(!turns[1].m_lanes[1].m_isActive, ());
}
} // namespace
