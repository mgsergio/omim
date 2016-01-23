#include "indexer/feature_meta.hpp"

#include "std/algorithm.hpp"
#include "std/target_os.hpp"

namespace feature
{

namespace
{
char constexpr const * kBaseWikiUrl =
#ifdef OMIM_OS_MOBILE
    ".m.wikipedia.org/wiki/";
#else
    ".wikipedia.org/wiki/";
#endif
} // namespace

string Metadata::GetWikiURL() const
{
  string v = this->Get(EType::FMD_WIKIPEDIA);
  if (v.empty())
    return v;

  auto const colon = v.find(':');
  if (colon == string::npos)
    return v;

  // Spaces and % sign should be replaced in urls.
  replace(v.begin() + colon, v.end(), ' ', '_');
  string::size_type percent, pos = colon;
  string const escapedPercent("%25");
  while ((percent = v.find('%', pos)) != string::npos)
  {
    v.replace(percent, 1, escapedPercent);
    pos = percent + escapedPercent.size();
  }
  // Trying to avoid redirects by constructing the right link.
  // TODO: Wikipedia article could be opened it a user's language, but need
  // generator level support to check for available article languages first.
  return "https://" + v.substr(0, colon) + kBaseWikiUrl + v.substr(colon + 1);
}

}  // namespace feature

// Prints types in osm-friendly format.
string DebugPrint(feature::Metadata::EType type)
{
  using feature::Metadata;
  switch (type)
  {
  case Metadata::EType::FMD_CUISINE: return "cuisine";
  case Metadata::EType::FMD_OPEN_HOURS: return "opening_hours";
  case Metadata::EType::FMD_PHONE_NUMBER: return "phone";
  case Metadata::EType::FMD_FAX_NUMBER: return "fax";
  case Metadata::EType::FMD_STARS: return "stars";
  case Metadata::EType::FMD_OPERATOR: return "operator";
  case Metadata::EType::FMD_URL: return "url";
  case Metadata::EType::FMD_WEBSITE: return "website";
  case Metadata::EType::FMD_INTERNET: return "internet_access";
  case Metadata::EType::FMD_ELE: return "elevation";
  case Metadata::EType::FMD_TURN_LANES: return "turn:lanes";
  case Metadata::EType::FMD_TURN_LANES_FORWARD: return "turn:lanes:forward";
  case Metadata::EType::FMD_TURN_LANES_BACKWARD: return "turn:lanes:backward";
  case Metadata::EType::FMD_EMAIL: return "email";
  case Metadata::EType::FMD_POSTCODE: return "addr:postcode";
  case Metadata::EType::FMD_WIKIPEDIA: return "wikipedia";
  case Metadata::EType::FMD_MAXSPEED: return "maxspeed";
  case Metadata::EType::FMD_FLATS: return "addr:flats";
  case Metadata::EType::FMD_HEIGHT: return "height";
  case Metadata::EType::FMD_MIN_HEIGHT: return "min_height";
  case Metadata::EType::FMD_DENOMINATION: return "denomination";
  case Metadata::EType::FMD_BUILDING_LEVELS: return "building:levels";
  case Metadata::EType::FMD_COUNT: CHECK(false, ("FMD_COUNT can not be used as a type."));
  };

  return string();
}
