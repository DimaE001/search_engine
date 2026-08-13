#include <string>

#include "gtest/gtest.h"
#include "news_dedupe/NewsDedupeEngine.h"
#include "news_dedupe/TextNormalizer.h"

namespace {

news_dedupe::NewsDocument makeDocument(
    const std::string& id,
    const std::string& source,
    const std::string& message_id,
    const std::string& text
) {
    news_dedupe::NewsDocument document;
    document.document_id = id;
    document.event_key = id;
    document.source = source;
    document.platform = "telegram";
    document.source_message_id = message_id;
    document.url = "https://t.me/" + source + "/" + message_id;
    document.published_at = "2026-08-13T10:00:00+00:00";
    document.lang = "ru";
    document.title = text;
    return document;
}

news_dedupe::NewsDedupeRequest makeRequest(
    const news_dedupe::NewsDocument& query,
    const news_dedupe::NewsDocument& document
) {
    news_dedupe::NewsDedupeRequest request;
    request.request_id = query.document_id;
    request.query = query;
    request.documents = {document};
    return request;
}

}  // namespace

TEST(TextNormalizerTest, NormalizesRussianAndUkrainianEquivalentText) {
    news_dedupe::TextNormalizer normalizer;

    EXPECT_EQ(
        normalizer.normalize("У Києві оголосили повітряну тривогу!"),
        "киев объявить воздушный тревога"
    );
    EXPECT_EQ(
        normalizer.normalize("В Киеве объявлена воздушная тревога."),
        "киев объявить воздушный тревога"
    );
}

TEST(TextNormalizerTest, KeepsMeaningfulModifiersOutOfStopWords) {
    news_dedupe::TextNormalizer normalizer;

    EXPECT_EQ(normalizer.normalize("Дом не разрушен"), "дом не разрушен");
    EXPECT_EQ(normalizer.normalize("Без пострадавших до 10 утра"), "без пострадавших до 10 утра");
    EXPECT_EQ(normalizer.normalize("Пострадавших нет"), "пострадавших без");
}

TEST(TextNormalizerTest, NormalizesEquivalentAbsenceMarkers) {
    news_dedupe::TextNormalizer normalizer;

    EXPECT_EQ(
        normalizer.extractNegations("Обошлось без пострадавших"),
        normalizer.extractNegations("Пострадавших нет")
    );
    EXPECT_EQ(
        normalizer.extractNegations("Постраждалих немає"),
        normalizer.extractNegations("Без постраждалих")
    );
}

TEST(NewsDedupeEngineTest, DetectsCrossSourceDuplicate) {
    auto query = makeDocument(
        "telegram:source_a/10",
        "source_a",
        "10",
        "У Києві оголосили повітряну тривогу"
    );
    query.lang = "uk";
    const auto document = makeDocument(
        "telegram:source_b/20",
        "source_b",
        "20",
        "В Киеве объявлена воздушная тревога"
    );

    const auto response = news_dedupe::NewsDedupeEngine().evaluate(makeRequest(query, document));

    EXPECT_EQ(response.decision, news_dedupe::NewsDecision::Duplicate);
    EXPECT_EQ(response.reason, "same_event");
    ASSERT_TRUE(response.best_match.has_value());
    EXPECT_DOUBLE_EQ(response.score, 1.0);
}

TEST(NewsDedupeEngineTest, PublishesUniqueNews) {
    const auto query = makeDocument(
        "telegram:source_a/10",
        "source_a",
        "10",
        "В Одессе открылся новый городской парк"
    );
    const auto document = makeDocument(
        "telegram:source_b/20",
        "source_b",
        "20",
        "В Киеве объявлена воздушная тревога"
    );

    const auto response = news_dedupe::NewsDedupeEngine().evaluate(makeRequest(query, document));

    EXPECT_EQ(response.decision, news_dedupe::NewsDecision::Publish);
    EXPECT_EQ(response.reason, "no_similar_event");
}

TEST(NewsDedupeEngineTest, DetectsChangedNumberInSameSourceMessage) {
    const auto old_document = makeDocument(
        "telegram:source_a/10",
        "source_a",
        "10",
        "После обстрела ранены 3 человека"
    );
    const auto query = makeDocument(
        "telegram:source_a/10",
        "source_a",
        "10",
        "После обстрела ранены 5 человек"
    );

    const auto response = news_dedupe::NewsDedupeEngine().evaluate(makeRequest(query, old_document));

    EXPECT_EQ(response.decision, news_dedupe::NewsDecision::Update);
    EXPECT_EQ(response.reason, "same_source_changed_facts");
    ASSERT_TRUE(response.best_match.has_value());
    ASSERT_EQ(response.best_match->changed_facts.size(), 1);
    EXPECT_EQ(response.best_match->changed_facts[0].type, "number");
    EXPECT_EQ(response.best_match->changed_facts[0].old_value, "3");
    EXPECT_EQ(response.best_match->changed_facts[0].new_value, "5");
}

TEST(NewsDedupeEngineTest, ReviewsConflictingNumbersFromDifferentSources) {
    const auto document = makeDocument(
        "telegram:source_a/10",
        "source_a",
        "10",
        "После обстрела ранены 3 человека"
    );
    const auto query = makeDocument(
        "telegram:source_b/20",
        "source_b",
        "20",
        "После обстрела ранены 5 человек"
    );

    const auto response = news_dedupe::NewsDedupeEngine().evaluate(makeRequest(query, document));

    EXPECT_EQ(response.decision, news_dedupe::NewsDecision::Review);
    EXPECT_EQ(response.reason, "conflicting_facts");
}

TEST(NewsDedupeEngineTest, ReviewsOppositeClaimsFromDifferentSources) {
    const auto document = makeDocument(
        "telegram:source_a/10",
        "source_a",
        "10",
        "После атаки дом разрушен"
    );
    const auto query = makeDocument(
        "telegram:source_b/20",
        "source_b",
        "20",
        "После атаки дом не разрушен"
    );

    const auto response = news_dedupe::NewsDedupeEngine().evaluate(makeRequest(query, document));

    EXPECT_EQ(response.decision, news_dedupe::NewsDecision::Review);
    EXPECT_EQ(response.reason, "conflicting_facts");
    ASSERT_TRUE(response.best_match.has_value());
    ASSERT_EQ(response.best_match->changed_facts.size(), 1);
    EXPECT_EQ(response.best_match->changed_facts[0].type, "negation");
    EXPECT_EQ(response.best_match->changed_facts[0].old_value, "affirmative");
    EXPECT_EQ(response.best_match->changed_facts[0].new_value, "not");
}

TEST(NewsDedupeEngineTest, RejectsEmptyQuery) {
    auto query = makeDocument("telegram:source_a/10", "source_a", "10", "");
    const auto document = makeDocument(
        "telegram:source_b/20",
        "source_b",
        "20",
        "В Киеве объявлена воздушная тревога"
    );

    EXPECT_THROW(
        news_dedupe::NewsDedupeEngine().evaluate(makeRequest(query, document)),
        std::invalid_argument
    );
}
