#ifndef NEWS_DEDUPE_ENGINE_H
#define NEWS_DEDUPE_ENGINE_H

#include <map>
#include <string>
#include <vector>

#include "news_dedupe/NewsDedupeModels.h"
#include "news_dedupe/TextNormalizer.h"

namespace news_dedupe {

class NewsDedupeEngine {
public:
    NewsDedupeResponse evaluate(const NewsDedupeRequest& request) const;

private:
    using TokenCounts = std::map<std::string, std::size_t>;
    using TokenWeights = std::map<std::string, double>;

    TextNormalizer normalizer_;

    static std::string documentText(const NewsDocument& document);
    static TokenCounts countTokens(const std::vector<std::string>& tokens);
    static bool hasSameSourceIdentity(const NewsDocument& query, const NewsDocument& document);
    static std::vector<ChangedFact> changedFacts(
        const std::vector<std::string>& old_numbers,
        const std::vector<std::string>& new_numbers,
        const std::vector<std::string>& old_negations,
        const std::vector<std::string>& new_negations
    );
    static double similarity(
        const TokenCounts& query,
        const TokenCounts& document,
        const TokenWeights& weights
    );
    static std::vector<std::string> commonTerms(
        const TokenCounts& query,
        const TokenCounts& document,
        const TokenWeights& weights
    );
};

}  // namespace news_dedupe

#endif
