#ifndef NEWS_DEDUPE_MODELS_H
#define NEWS_DEDUPE_MODELS_H

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace news_dedupe {

inline constexpr const char* kProtocolVersion = "1.0";
inline constexpr const char* kEngineName = "search_engine";
inline constexpr const char* kEngineVersion = "news-dedupe-1";

struct NewsDocument {
    std::string document_id;
    std::string event_key;
    std::string source;
    std::string platform;
    std::string source_message_id;
    std::string url;
    std::string published_at;
    std::string lang;
    std::string title;
    std::string text;
    std::string telegram_message_id;
};

struct NewsDedupeOptions {
    std::size_t top_k = 5;
    double duplicate_threshold = 0.85;
    double review_threshold = 0.65;
};

struct NewsDedupeRequest {
    std::string protocol_version = kProtocolVersion;
    std::string request_id;
    NewsDocument query;
    std::vector<NewsDocument> documents;
    NewsDedupeOptions options;
};

struct ChangedFact {
    std::string type;
    std::string old_value;
    std::string new_value;
};

struct NewsMatch {
    std::string document_id;
    std::string event_key;
    double score = 0.0;
    std::vector<std::string> common_terms;
    std::vector<ChangedFact> changed_facts;
};

enum class NewsDecision {
    Publish,
    Duplicate,
    Update,
    Review,
};

struct NewsDedupeResponse {
    std::string protocol_version = kProtocolVersion;
    std::string request_id;
    NewsDecision decision = NewsDecision::Publish;
    double score = 0.0;
    std::string reason = "no_similar_event";
    std::optional<NewsMatch> best_match;
    std::vector<NewsMatch> matches;
};

const char* decisionToString(NewsDecision decision);

}  // namespace news_dedupe

#endif
