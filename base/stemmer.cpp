#include "stemmer.hpp"
#include "base/assert.hpp"
#include "platform/platform.hpp"


namespace my
{
Stemmer::Stemmer(string const & locale, string const & dictsPath):
    m_dictsPath(dictsPath),
    m_locale(locale)
{
  string const affDict = MakeAffPath(locale, m_dictsPath);
  string const wordDict = MakeDicPath(locale, m_dictsPath);

  ASSERT(DictExists(affDict), affDict + " no such file");
  ASSERT(DictExists(wordDict), wordDict + " no such file");

  m_stemmerImpl.reset(new TStemmerImpl(affDict.data(), wordDict.data()));
  VERIFY(GetDictEncoding() == "UTF-8",
         "Wrong dictionary encoding " + GetDictEncoding() + "for locale " + m_locale );
}

string Stemmer::Stem(string const & word) const
{
  char ** slist;
  int const stemsCount = m_stemmerImpl->stem(&slist, word.data());
  if (stemsCount == 0)
    return {};

  string const stem{slist[0]};
  m_stemmerImpl->free_list(&slist, stemsCount);
  return stem;
}

string Stemmer::GetDictEncoding() const
{
  return m_stemmerImpl->get_dic_encoding();
}

bool Stemmer::DictsExist(string const & locale, string const & dictsPath)
{
  string const affDict = MakeAffPath(locale, dictsPath);
  string const wordDict = MakeDicPath(locale, dictsPath);
  return DictExists(affDict) && DictExists(wordDict);
}

bool Stemmer::DictExists(string const & path)
{
  return Platform::IsFileExistsByFullPath(path);
}

string Stemmer::MakeAffPath(string const & locale, string const & dictsPath)
{
  return dictsPath + "/" + locale + ".aff";
}

string Stemmer::MakeDicPath(string const & locale, string const & dictsPath)
{
  return dictsPath + "/" + locale + ".dic";
}
}// namespace my
