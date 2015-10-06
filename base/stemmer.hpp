#pragma once

#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "3party/hunspell/src/hunspell/hunspell.hxx"


namespace my
{
class Stemmer
{
  using TStemmerImpl = Hunspell;
  using TStemmerImplPtr = std::unique_ptr<TStemmerImpl>;

 public:
  Stemmer(string const & locale, string const & dictsPath);
  Stemmer(Stemmer const &) = delete;
  Stemmer(Stemmer &&) = default;

  Stemmer & operator=(Stemmer const &) = delete;
  Stemmer & operator=(Stemmer &&) = delete;

  string Stem(string const & word) const;

 private:
  string GetDictEncoding() const;
  string InputToDictEncoding(string const & word) const;
  string DictToInputEncoding(string const & word) const;

  bool DictExists(string const & path); // const ?

 private:
  // @TODO(mgsergio) get from env/config/whatever
  string const m_dictsPath;
  string const m_locale;

  TStemmerImplPtr m_stemmerImpl;
};
} // namespace my
