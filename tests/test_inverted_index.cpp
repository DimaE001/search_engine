#include "gtest/gtest.h"
#include "InvertedIndex.h"


// одно слово
TEST(InvertedIndexTest, WordCountBasic)
{
    // 1. Arrange (подготовка данных)
    std::vector<std::string> docs = {
        "milk milk water",
        "milk sugar"
    };

    InvertedIndex index;
    index.UpdateDocumentBase(docs);

    // 2. Act (вызов функции)
    auto result = index.GetWordCount("milk");

    // 3. Assert (проверка результата)
    std::vector<Entry> expected = {
        {0, 2},
        {1, 1}
    };

    EXPECT_EQ(result, expected);
}

// отсутствует слово
TEST(InvertedIndexTest, WordNotFound)
{
    std::vector<std::string> docs = {
        "milk water",
        "sugar tea"
    };

    InvertedIndex index;
    index.UpdateDocumentBase(docs);

    auto result = index.GetWordCount("coffee");

    EXPECT_TRUE(result.empty());
}

// постор слова
TEST(InvertedIndexTest, WordRepeated)
{
    std::vector<std::string> docs = {
        "apple apple apple"
    };

    InvertedIndex index;
    index.UpdateDocumentBase(docs);

    auto result = index.GetWordCount("apple");

    std::vector<Entry> expected = {
        {0,3}
    };

    EXPECT_EQ(result, expected);
}

//слово есть в одном доке
TEST(InvertedIndexTest, UpdateDocumentBaseOne)
{
    std::vector<std::string> docs = {
        "milk milk water",
        "water sugar"
    };

    InvertedIndex index;

    // строим индекс
    index.UpdateDocumentBase(docs);

    // проверяем слово milk
    auto result = index.GetWordCount("milk");

    std::vector<Entry> expected = {
        {0,2}
    };

    EXPECT_EQ(result, expected);
}

// отсутствуют доки
TEST(InvertedIndexTest, EmptyDocuments)
{
    std::vector<std::string> docs;

    InvertedIndex index;
    index.UpdateDocumentBase(docs);

    auto result = index.GetWordCount("milk");

    EXPECT_TRUE(result.empty());
}

//  переиндексация
TEST(InvertedIndexTest, RebuildIndex)
{
    InvertedIndex index;

    std::vector<std::string> docs1 = {
        "milk water"
    };

    index.UpdateDocumentBase(docs1);

    auto res1 = index.GetWordCount("milk");
    ASSERT_EQ(res1.size(), 1);

    std::vector<std::string> docs2 = {
        "apple juice"
    };

    index.UpdateDocumentBase(docs2);

    auto res2 = index.GetWordCount("milk");

    EXPECT_TRUE(res2.empty());
}

// много документов
TEST(InvertedIndexTest, ManyDocuments)
{
    std::vector<std::string> docs = {
        "milk  water wate rwater ",
        "milk sugar sugar",
        "milk tea",
        "milk"
    };

    InvertedIndex index;
    index.UpdateDocumentBase(docs);

    auto result = index.GetWordCount("milk");

    std::sort(result.begin(), result.end(),
        [](const Entry& a, const Entry& b) {
            return a.doc_id < b.doc_id;
        });

    std::vector<Entry> expected = {
        {0,1},
        {1,1},
        {2,1},
        {3,1}
    };

    EXPECT_EQ(result, expected);
}