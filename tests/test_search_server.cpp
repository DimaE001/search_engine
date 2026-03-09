#include "gtest/gtest.h"
#include <string>
#include "SearchServer.h"

// задан макс выдача
TEST(SearchServerTest, MaxResponsesLimit)
{
    std::vector<std::string> docs = {
        "milk",
        "milk",
        "milk",
        "milk"
    };

    InvertedIndex index;
    index.UpdateDocumentBase(docs);

    SearchServer server(index, 2);

    std::vector<std::string> query = {"milk"};

    auto result = server.search(query);

    EXPECT_EQ(result[0].size(), 2);
}

// пустой результат
TEST(SearchServerTest, NoResults)
{
    std::vector<std::string> docs = {
        "milk water"
    };

    InvertedIndex index;
    index.UpdateDocumentBase(docs);

    SearchServer server(index, 5);

    std::vector<std::string> query = {
        "coffee"
    };

    auto result = server.search(query);

    EXPECT_TRUE(result[0].empty());
}

// базовый поиск результат
TEST(SearchServerTest, BasicSearch)
{
    std::vector<std::string> docs = {
        "milk milk water",
        "milk sugar"
    };

    InvertedIndex index;
    index.UpdateDocumentBase(docs);

    SearchServer server(index, 5);

    std::vector<std::string> query = {
        "milk"
    };

    auto result = server.search(query);

    std::vector<std::vector<RelativeIndex>> expected = {
        {
            {0,1.0},
            {1,0.5}
        }
    };

    EXPECT_EQ(result, expected);
}

// объединение при выдаче (пересечение)
TEST(SearchServerTest, AndSearch)
{
    std::vector<std::string> docs = {
        "milk water",
        "milk sugar",
        "water sugar"
    };

    InvertedIndex index;
    index.UpdateDocumentBase(docs);

    SearchServer server(index, 5);

    std::vector<std::string> query = {
        "milk water"
    };

    auto result = server.search(query);

    std::vector<std::vector<RelativeIndex>> expected = {
        {
            {0,1.0}
        }
    };

    EXPECT_EQ(result, expected);
}

// rank <  /  > docid
TEST(SearchServerTest, SortByDocIdIfRankEqual)
{
    std::vector<std::string> docs = {
        "milk",
        "milk",
        "milk"
    };

    InvertedIndex index;
    index.UpdateDocumentBase(docs);

    SearchServer server(index, 5);

    std::vector<std::string> query = {"milk"};

    auto result = server.search(query);

    std::vector<std::vector<RelativeIndex>> expected = {
        {
            {0,1.0},
            {1,1.0},
            {2,1.0}
        }
    };

    EXPECT_EQ(result, expected);
}

// несколько запросов
TEST(SearchServerTest, MultipleQueries)
{
    std::vector<std::string> docs = {
        "milk water",
        "sugar tea"
    };

    InvertedIndex index;
    index.UpdateDocumentBase(docs);

    SearchServer server(index, 5);

    std::vector<std::string> query = {
        "milk",
        "tea"
    };

    auto result = server.search(query);

    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0][0].doc_id, 0);
    EXPECT_EQ(result[1][0].doc_id, 1);
}

//  несколько  повторяющихся слов в запрсе
TEST(SearchServerTest, RepeatedWordsInQuery)
{
    std::vector<std::string> docs = {
        "milk water",
        "milk"
    };

    InvertedIndex index;
    index.UpdateDocumentBase(docs);

    SearchServer server(index, 5);

    std::vector<std::string> query = {"milk milk milk"};

    auto result = server.search(query);

    EXPECT_EQ(result[0].size(), 2);
}

// пустой запрос - пустой результат
TEST(SearchServerTest, EmptyQuery)
{
    std::vector<std::string> docs = {
        "milk water",
        "sugar tea"
    };

    InvertedIndex index;
    index.UpdateDocumentBase(docs);

    SearchServer server(index, 5);

    std::vector<std::string> query = {""};

    auto result = server.search(query);

    ASSERT_EQ(result.size(), 1);
    EXPECT_TRUE(result[0].empty());
}

//  запрос только символов
TEST(SearchServerTest, QueryOnlySymbols)
{
    std::vector<std::string> docs = {
        "milk water"
    };

    InvertedIndex index;
    index.UpdateDocumentBase(docs);

    SearchServer server(index, 5);

    std::vector<std::string> query = {
        "!!! @@ ###"
    };

    auto result = server.search(query);

    ASSERT_EQ(result.size(), 1);
    EXPECT_TRUE(result[0].empty());
}

//  есть первое слово , но and  всего запроса нет
TEST(SearchServerTest, AndSearchNoIntersection)
{
    std::vector<std::string> docs = {
        "milk",
        "water",
        "sugar"
    };

    InvertedIndex index;
    index.UpdateDocumentBase(docs);

    SearchServer server(index, 5);

    std::vector<std::string> query = {
        "milk water"
    };

    auto result = server.search(query);

    ASSERT_EQ(result.size(), 1);
    EXPECT_TRUE(result[0].empty());
}

//есть но не во всех доках
TEST(SearchServerTest, WordInSomeDocuments)
{
    std::vector<std::string> docs = {
        "milk water",
        "water",
        "milk"
    };

    InvertedIndex index;
    index.UpdateDocumentBase(docs);

    SearchServer server(index, 5);

    std::vector<std::string> query = {"milk"};

    auto result = server.search(query);

    ASSERT_EQ(result[0].size(), 2);
}