#include "i18n/tr.hpp"
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv) {
 try {
  if(argc==3 && std::string(argv[2])=="--discovery-only") {
   if(i18n::i18nGetLanguageName(argv[1]).empty())throw std::runtime_error("Missing catalog label");
   if(i18n::_("Language")!="Language")throw std::runtime_error("Discovery changed selected language");
   std::cout<<"Catalog discovery without a non-C locale passed\n";return 0;
  }
  if(argc!=2 || !i18n::i18nLoadLanguage(argv[1])) throw std::runtime_error("Catalog load failed");
  if(i18n::_("Language")!="言語") throw std::runtime_error("C locale suppressed explicit translation");
  auto held=i18n::__("Language");
  for(int i=0;i<10000;++i)if(held!=i18n::__("Language"))throw std::runtime_error("Translation was not interned");
  for(int i=0;i<1000;++i)i18n::__(std::to_string(i));
  i18n::i18nClearLanguage();
  if(std::string(held)!="言語" || i18n::_("Language")!="Language")throw std::runtime_error("Pointer lifetime or locale restore failed");
  std::cout<<"Explicit translation under C locale and stable string interning passed\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
