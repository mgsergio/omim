#pragma once

#include <chrono>
#include <cstdint>

namespace openlr
{
namespace v2
{
struct Stats
{
  void Add(Stats const & s)
  {
    m_routesHandled += s.m_routesHandled;
    m_routesFailed += s.m_routesFailed;
    m_noCandidateFound += s.m_noCandidateFound;
    m_noShortestPathFound += s.m_noShortestPathFound;
    m_wrongOffsets += s.m_wrongOffsets;
    m_dnpIsZero += s.m_dnpIsZero;
  }

  uint32_t m_routesHandled = 0;
  uint32_t m_routesFailed = 0;
  uint32_t m_noCandidateFound = 0;
  uint32_t m_noShortestPathFound = 0;
  uint32_t m_wrongOffsets = 0;
  // Number of zeroed distance-to-next point values in the input.
  uint32_t m_dnpIsZero = 0;
};
}  // namespace V2
}  // namespace openlr
