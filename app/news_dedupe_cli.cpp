#include <iostream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

#include "news_dedupe/NewsDedupeEngine.h"
#include "news_dedupe/NewsDedupeModels.h"

namespace {

using json = nlohmann::json;
using news_dedupe::ChangedFact;
using news_dedupe::NewsDocument;
using news_dedupe::NewsDedupeRequest;
using news_dedupe::NewsDedupeResponse;
using news_dedupe::NewsMatch;

std::string requiredString(const json& object, const char* field) {
    if (!object.contains(field) || !object.at(field).is_string()) {
        throw std::invalid_argument(std::string("missing or invalid string field: ") + field);
    }
    return object.at(field).get<std::string>();
}

std::string optionalString(const json& object, const char* field) {
    if (!object.contains(field) || object.at(field).is_null()) {
        return "";
    }
    if (!object.at(field).is_string()) {
        throw std::invalid_argument(std::string("invalid string field: ") + field);
    }
    return object.at(field).get<std::string>();
}

NewsDocument parseDocument(const json& value, bool require_event_key) {
    if (!value.is_object()) {
        throw std::invalid_argument("news document must be an object");
    }

    NewsDocument document;
    document.document_id = requiredString(value, "document_id");
    document.event_key = require_event_key
        ? requiredString(value, "event_key")
        : optionalString(value, "event_key");
    document.source = requiredString(value, "source");
    document.platform = requiredString(value, "platform");
    document.source_message_id = optionalString(value, "source_message_id");
    document.url = requiredString(value, "url");
    document.published_at = requiredString(value, "published_at");
    document.lang = optionalString(value, "lang");
    document.title = requiredString(value, "title");
    document.text = requiredString(value, "text");
    document.telegram_message_id = optionalString(value, "telegram_message_id");
    return document;
}

NewsDedupeRequest parseRequest(const json& value) {
    if (!value.is_object()) {
        throw std::invalid_argument("request must be an object");
    }

    NewsDedupeRequest request;
    request.protocol_version = requiredString(value, "protocol_version");
    request.request_id = requiredString(value, "request_id");
    if (request.protocol_version != news_dedupe::kProtocolVersion) {
        throw std::domain_error("unsupported protocol version");
    }
    if (!value.contains("query")) {
        throw std::invalid_argument("missing query");
    }
    request.query = parseDocument(value.at("query"), false);

    if (!value.contains("documents") || !value.at("documents").is_array()) {
        throw std::invalid_argument("documents must be an array");
    }
    for (const auto& document : value.at("documents")) {
        request.documents.push_back(parseDocument(document, true));
    }

    if (value.contains("options")) {
        const auto& options = value.at("options");
        if (!options.is_object()) {
            throw std::invalid_argument("options must be an object");
        }
        if (options.contains("top_k")) {
            if (!options.at("top_k").is_number_integer()) {
                throw std::invalid_argument("options.top_k must be an integer");
            }
            const auto top_k = options.at("top_k").get<long long>();
            if (top_k <= 0) {
                throw std::invalid_argument("options.top_k must be greater than zero");
            }
            request.options.top_k = static_cast<std::size_t>(top_k);
        }
        if (options.contains("duplicate_threshold")) {
            request.options.duplicate_threshold = options.at("duplicate_threshold").get<double>();
        }
        if (options.contains("review_threshold")) {
            request.options.review_threshold = options.at("review_threshold").get<double>();
        }
    }
    return request;
}

json changedFactJson(const ChangedFact& fact) {
    return {
        {"type", fact.type},
        {"old", fact.old_value},
        {"new", fact.new_value},
    };
}

json matchJson(const NewsMatch& match, bool include_changed_facts) {
    json value = {
        {"document_id", match.document_id},
        {"event_key", match.event_key},
        {"score", match.score},
        {"common_terms", match.common_terms},
    };
    if (include_changed_facts) {
        value["changed_facts"] = json::array();
        for (const auto& fact : match.changed_facts) {
            value["changed_facts"].push_back(changedFactJson(fact));
        }
    }
    return value;
}

json responseJson(const NewsDedupeResponse& response) {
    json value = {
        {"protocol_version", response.protocol_version},
        {"request_id", response.request_id},
        {"decision", news_dedupe::decisionToString(response.decision)},
        {"score", response.score},
        {"reason", response.reason},
        {"matches", json::array()},
        {"engine", {
            {"name", news_dedupe::kEngineName},
            {"version", news_dedupe::kEngineVersion},
        }},
    };
    if (response.best_match.has_value()) {
        value["best_match"] = matchJson(*response.best_match, true);
    }
    for (const auto& match : response.matches) {
        value["matches"].push_back(matchJson(match, false));
    }
    return value;
}

json errorJson(
    const std::string& request_id,
    const std::string& code,
    const std::string& message
) {
    return {
        {"protocol_version", news_dedupe::kProtocolVersion},
        {"request_id", request_id},
        {"error", {
            {"code", code},
            {"message", message},
        }},
        {"engine", {
            {"name", news_dedupe::kEngineName},
            {"version", news_dedupe::kEngineVersion},
        }},
    };
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string request_id;
    try {
        if (argc != 2 || std::string(argv[1]) != "--news-dedupe-stdin") {
            throw std::invalid_argument("expected --news-dedupe-stdin");
        }

        json input;
        try {
            std::cin >> input;
        } catch (const json::parse_error& error) {
            std::cout << errorJson("", "invalid_json", error.what()).dump() << '\n';
            return 2;
        }
        if (input.is_object() && input.contains("request_id") && input.at("request_id").is_string()) {
            request_id = input.at("request_id").get<std::string>();
        }

        const auto request = parseRequest(input);
        news_dedupe::NewsDedupeEngine engine;
        std::cout << responseJson(engine.evaluate(request)).dump() << '\n';
        return 0;
    } catch (const std::domain_error& error) {
        std::cout << errorJson(request_id, "unsupported_protocol", error.what()).dump() << '\n';
        return 2;
    } catch (const std::invalid_argument& error) {
        std::cout << errorJson(request_id, "invalid_request", error.what()).dump() << '\n';
        return 2;
    } catch (const std::exception& error) {
        std::cout << errorJson(request_id, "internal_error", error.what()).dump() << '\n';
        return 1;
    }
}
