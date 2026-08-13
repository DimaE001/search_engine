#include "news_dedupe/NewsDedupeEngine.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>
#include <stdexcept>

namespace news_dedupe {
namespace {

bool isNumberToken(const std::string& token) {
    return !token.empty() && std::all_of(token.begin(), token.end(), [](unsigned char value) {
        return std::isdigit(value) != 0;
    });
}

}  // namespace

const char* decisionToString(NewsDecision decision) {
    switch (decision) {
        case NewsDecision::Publish: return "publish";
        case NewsDecision::Duplicate: return "duplicate";
        case NewsDecision::Update: return "update";
        case NewsDecision::Review: return "review";
    }
    return "review";
}

std::string NewsDedupeEngine::documentText(const NewsDocument& document) {
    if (document.title.empty()) {
        return document.text;
    }
    if (document.text.empty()) {
        return document.title;
    }
    return document.title + " " + document.text;
}

NewsDedupeEngine::TokenCounts NewsDedupeEngine::countTokens(
    const std::vector<std::string>& tokens
) {
    TokenCounts counts;
    for (const auto& token : tokens) {
        if (!isNumberToken(token)) {
            ++counts[token];
        }
    }
    return counts;
}

bool NewsDedupeEngine::hasSameSourceIdentity(
    const NewsDocument& query,
    const NewsDocument& document
) {
    if (query.source != document.source || query.platform != document.platform) {
        return false;
    }
    if (!query.source_message_id.empty() && !document.source_message_id.empty()) {
        return query.source_message_id == document.source_message_id;
    }
    return !query.url.empty() && query.url == document.url;
}

std::vector<ChangedFact> NewsDedupeEngine::changedFacts(
    const std::vector<std::string>& old_numbers,
    const std::vector<std::string>& new_numbers,
    const std::vector<std::string>& old_negations,
    const std::vector<std::string>& new_negations
) {
    std::vector<ChangedFact> changes;
    const std::size_t count = std::max(old_numbers.size(), new_numbers.size());
    changes.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        const std::string old_value = index < old_numbers.size() ? old_numbers[index] : "";
        const std::string new_value = index < new_numbers.size() ? new_numbers[index] : "";
        if (old_value != new_value) {
            changes.push_back({"number", old_value, new_value});
        }
    }
    if (old_negations != new_negations) {
        const std::string old_value = old_negations.empty() ? "affirmative" : old_negations.front();
        const std::string new_value = new_negations.empty() ? "affirmative" : new_negations.front();
        changes.push_back({"negation", old_value, new_value});
    }
    return changes;
}

double NewsDedupeEngine::similarity(
    const TokenCounts& query,
    const TokenCounts& document,
    const TokenWeights& weights
) {
    if (query.empty() || document.empty()) {
        return 0.0;
    }

    double intersection = 0.0;
    double union_weight = 0.0;
    double query_weight = 0.0;
    double document_weight = 0.0;
    double dot = 0.0;
    double query_norm = 0.0;
    double document_norm = 0.0;

    std::set<std::string> all_tokens;
    for (const auto& [token, count] : query) {
        all_tokens.insert(token);
        const double weighted = static_cast<double>(count) * weights.at(token);
        query_norm += weighted * weighted;
    }
    for (const auto& [token, count] : document) {
        all_tokens.insert(token);
        const double weighted = static_cast<double>(count) * weights.at(token);
        document_norm += weighted * weighted;
    }

    for (const auto& token : all_tokens) {
        const auto query_it = query.find(token);
        const auto document_it = document.find(token);
        const std::size_t query_count = query_it == query.end() ? 0 : query_it->second;
        const std::size_t document_count = document_it == document.end() ? 0 : document_it->second;
        const double weight = weights.at(token);

        intersection += static_cast<double>(std::min(query_count, document_count)) * weight;
        union_weight += static_cast<double>(std::max(query_count, document_count)) * weight;
        query_weight += static_cast<double>(query_count) * weight;
        document_weight += static_cast<double>(document_count) * weight;
        dot += static_cast<double>(query_count * document_count) * weight * weight;
    }

    const double jaccard = union_weight > 0.0 ? intersection / union_weight : 0.0;
    const double cosine = query_norm > 0.0 && document_norm > 0.0
        ? dot / (std::sqrt(query_norm) * std::sqrt(document_norm))
        : 0.0;
    const double coverage_base = std::min(query_weight, document_weight);
    const double coverage = coverage_base > 0.0 ? intersection / coverage_base : 0.0;
    return std::clamp(0.45 * jaccard + 0.35 * cosine + 0.20 * coverage, 0.0, 1.0);
}

std::vector<std::string> NewsDedupeEngine::commonTerms(
    const TokenCounts& query,
    const TokenCounts& document,
    const TokenWeights& weights
) {
    std::vector<std::string> common;
    for (const auto& [token, count] : query) {
        if (document.find(token) != document.end()) {
            common.push_back(token);
        }
    }
    std::sort(common.begin(), common.end(), [&weights](const auto& left, const auto& right) {
        if (weights.at(left) == weights.at(right)) {
            return left < right;
        }
        return weights.at(left) > weights.at(right);
    });
    return common;
}

NewsDedupeResponse NewsDedupeEngine::evaluate(const NewsDedupeRequest& request) const {
    if (request.protocol_version != kProtocolVersion) {
        throw std::invalid_argument("unsupported protocol version");
    }
    if (request.query.title.empty() && request.query.text.empty()) {
        throw std::invalid_argument("query.title and query.text cannot both be empty");
    }
    if (request.options.top_k == 0) {
        throw std::invalid_argument("options.top_k must be greater than zero");
    }
    if (request.options.review_threshold < 0.0 ||
        request.options.duplicate_threshold > 1.0 ||
        request.options.review_threshold > request.options.duplicate_threshold) {
        throw std::invalid_argument("invalid similarity thresholds");
    }

    NewsDedupeResponse response;
    response.request_id = request.request_id;

    const std::string query_text = documentText(request.query);
    const auto query_tokens = normalizer_.tokenize(query_text);
    const auto query_counts = countTokens(query_tokens);
    if (query_counts.empty()) {
        response.decision = NewsDecision::Review;
        response.reason = "insufficient_text";
        return response;
    }

    std::vector<TokenCounts> document_counts;
    document_counts.reserve(request.documents.size());
    std::map<std::string, std::size_t> document_frequency;
    for (const auto& document : request.documents) {
        auto counts = countTokens(normalizer_.tokenize(documentText(document)));
        for (const auto& [token, count] : counts) {
            ++document_frequency[token];
        }
        document_counts.push_back(std::move(counts));
    }

    TokenWeights weights;
    std::set<std::string> all_tokens;
    for (const auto& [token, count] : query_counts) {
        all_tokens.insert(token);
    }
    for (const auto& counts : document_counts) {
        for (const auto& [token, count] : counts) {
            all_tokens.insert(token);
        }
    }
    const double document_total = static_cast<double>(request.documents.size());
    for (const auto& token : all_tokens) {
        const double frequency = static_cast<double>(document_frequency[token]);
        weights[token] = std::log((document_total + 1.0) / (frequency + 1.0)) + 1.0;
    }

    std::optional<NewsMatch> identity_match;
    const auto query_numbers = normalizer_.extractNumbers(query_text);
    const auto query_negations = normalizer_.extractNegations(query_text);
    for (std::size_t index = 0; index < request.documents.size(); ++index) {
        const auto& document = request.documents[index];
        NewsMatch match;
        match.document_id = document.document_id;
        match.event_key = document.event_key;
        match.score = similarity(query_counts, document_counts[index], weights);
        match.common_terms = commonTerms(query_counts, document_counts[index], weights);
        match.changed_facts = changedFacts(
            normalizer_.extractNumbers(documentText(document)),
            query_numbers,
            normalizer_.extractNegations(documentText(document)),
            query_negations
        );

        if (hasSameSourceIdentity(request.query, document)) {
            identity_match = match;
        }
        if (match.score > 0.0) {
            response.matches.push_back(std::move(match));
        }
    }

    std::sort(response.matches.begin(), response.matches.end(), [](const auto& left, const auto& right) {
        if (left.score == right.score) {
            return left.document_id < right.document_id;
        }
        return left.score > right.score;
    });
    if (response.matches.size() > request.options.top_k) {
        response.matches.resize(request.options.top_k);
    }

    if (identity_match.has_value()) {
        response.best_match = identity_match;
        response.score = identity_match->score;
        if (!identity_match->changed_facts.empty() &&
            identity_match->score >= request.options.review_threshold) {
            response.decision = NewsDecision::Update;
            response.reason = "same_source_changed_facts";
        } else if (identity_match->score >= request.options.duplicate_threshold) {
            response.decision = NewsDecision::Duplicate;
            response.reason = "same_source_unchanged";
        } else {
            response.decision = NewsDecision::Review;
            response.reason = identity_match->changed_facts.empty()
                ? "possible_same_event"
                : "conflicting_facts";
        }
        return response;
    }

    if (response.matches.empty()) {
        return response;
    }

    response.best_match = response.matches.front();
    response.score = response.best_match->score;
    if (!response.best_match->changed_facts.empty() &&
        response.score >= request.options.review_threshold) {
        response.decision = NewsDecision::Review;
        response.reason = "conflicting_facts";
    } else if (response.score >= request.options.duplicate_threshold) {
        response.decision = NewsDecision::Duplicate;
        response.reason = "same_event";
    } else if (response.score >= request.options.review_threshold) {
        response.decision = NewsDecision::Review;
        response.reason = "possible_same_event";
    }
    return response;
}

}  // namespace news_dedupe
