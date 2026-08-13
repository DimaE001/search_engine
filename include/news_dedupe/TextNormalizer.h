#ifndef NEWS_DEDUPE_TEXT_NORMALIZER_H
#define NEWS_DEDUPE_TEXT_NORMALIZER_H

#include <string>
#include <vector>

namespace news_dedupe {

class TextNormalizer {
public:
    std::string normalize(const std::string& text) const;
    std::vector<std::string> tokenize(const std::string& text) const;
    std::vector<std::string> extractNumbers(const std::string& text) const;
};

}  // namespace news_dedupe

#endif
