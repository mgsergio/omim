#include "stemmer.hpp"
#include "base/assert.hpp"
#include "platform/platform.hpp"


namespace my
{
Stemmer::Stemmer(string const & locale, string const & dictsPath):
    m_dictsPath(dictsPath),
    m_locale(locale)
{
  string const affDict = m_dictsPath + locale + ".aff";
  string const wordDict = m_dictsPath + locale + ".dic";

  ASSERT(DictExists(affDict), affDict + " no such file");
  ASSERT(DictExists(wordDict), wordDict + " no such file");

  m_stemmerImpl.reset(new TStemmerImpl(affDict.data(), wordDict.data()));
  VERIFY(GetDictEncoding() == "UTF-8",
         "Wrong dictionary encoding " + GetDictEncoding() + "for locale " + m_locale );
}

string Stemmer::Stem(string const & word) const
{
  char ** slist;
  int const stemsCount = m_stemmerImpl->stem(&slist, InputToDictEncoding(word).data());
  if (stemsCount == 0)
    return {};

  string const stem{slist[0]};
  m_stemmerImpl->free_list(&slist, stemsCount);
  return DictToInputEncoding(stem);
}

string Stemmer::GetDictEncoding() const
{
  return m_stemmerImpl->get_dic_encoding();
}

bool Stemmer::DictExists(string const & path)
{
  return Platform::IsFileExistsByFullPath(path);
}

string Stemmer::InputToDictEncoding(string const & word) const
{
  return "";
}

string Stemmer::DictToInputEncoding(string const & word) const
{
  return "";
}
}// namespace my
