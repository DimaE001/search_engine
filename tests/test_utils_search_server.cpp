#include <cstddef>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "utilsSearchServer.h"
#include "constantsSearchServer.h"


/// для  get uniq
TEST(UtilsTest, UniqWordsBasic)
{
    std::vector<std::string> queries = {
        "milk milk water"
    };

    auto result = getUniqWords(queries);

    std::vector<std::string> expected = {
        "milk water"
    };

    EXPECT_EQ(result, expected);
}

//несколько запросов
TEST(UtilsTest, MultipleQueries)
{
    std::vector<std::string> queries = {
        "milk milk water",
        "apple apple juice"
    };

    auto result = getUniqWords(queries);

    std::vector<std::string> expected = {
        "milk water",
        "apple juice"
    };

    EXPECT_EQ(result, expected);
}

// рустой запрос
TEST(UtilsTest, EmptyQuery)
{
    std::vector<std::string> queries = {
        ""
    };

    auto result = getUniqWords(queries);

    std::vector<std::string> expected = {
        ""
    };

    EXPECT_EQ(result, expected);
}

// повторяюзиеся слова
TEST(UtilsTest, OnlyRepeatedWords)
{
    std::vector<std::string> queries = {
        "apple apple apple"
    };

    auto result = getUniqWords(queries);

    std::vector<std::string> expected = {
        "apple"
    };

    EXPECT_EQ(result, expected);
}

// валидный документ для поиска
TEST(UtilsTest, ValidDocumentBasic)
{
    std::string text = "milk water sugar";
    EXPECT_TRUE(isValidDocument(text));
}

// пустой документ формально валиден для корректной работы
TEST(UtilsTest, ValidDocumentEmpty)
{
    std::string text = "";
    EXPECT_TRUE(isValidDocument(text));
}

// слово длиннее MAX_DOCUMENT_WORD_LENGTH символов
TEST(UtilsTest, InvalidDocumentWordTooLong)
{
    size_t toLongMAX_DOCUMENT_WORD_LENGTH  = MAX_DOCUMENT_WORD_LENGTH +1;
    std::string long_word(toLongMAX_DOCUMENT_WORD_LENGTH, 'a');
    EXPECT_FALSE(isValidDocument(long_word));
}

// документ содержит больше toLongMAX_DOCUMENT_WORDS слов
TEST(UtilsTest, InvalidDocumentTooManyWords)
{
    size_t toLongMAX_DOCUMENT_WORDS = MAX_DOCUMENT_WORDS + 1;
    std::string text;
    for (size_t i = 0; i < toLongMAX_DOCUMENT_WORDS; ++i) {
        if (!text.empty()) text += ' ';
        text += "milk";
    }

    EXPECT_FALSE(isValidDocument(text));
}

// документ содержит заглавную букву
TEST(UtilsTest, InvalidDocumentUppercaseLetter)
{
    std::string text = "Milk water";
    EXPECT_FALSE(isValidDocument(text));
}

// документ содержит цифру
TEST(UtilsTest, InvalidDocumentDigit)
{
    std::string text = "milk1 water";
    EXPECT_FALSE(isValidDocument(text));
}

// документ содержит знак препинания
TEST(UtilsTest, InvalidDocumentPunctuation)
{
    std::string text = "milk, water";
    EXPECT_FALSE(isValidDocument(text));
}

// несколько пробелов между словами - валидно
TEST(UtilsTest, ValidDocumentMultipleSpaces)
{
    std::string text = "milk    water     sugar";
    EXPECT_TRUE(isValidDocument(text));
}

// длина слова ровно MAX_DOCUMENT_WORD_LENGTH символов - валидно
TEST(UtilsTest, ValidDocumentWordLengthMAX_DOCUMENT_WORD_LENGTH)
{
    std::string word(MAX_DOCUMENT_WORD_LENGTH, 'a');
    EXPECT_TRUE(isValidDocument(word));
}

// ровно MAX_DOCUMENT_WORDS слов - валидно
TEST(UtilsTest, ValidDocumentExactlyMAX_DOCUMENT_WORDS)
{
    std::string text;
    for (size_t i = 0; i < MAX_DOCUMENT_WORDS; ++i) {
        if (!text.empty()) text += ' ';
        text += "milk";
    }

    EXPECT_TRUE(isValidDocument(text));
}

//одни пробелы
TEST(UtilsTest, ValidDocumentOnlySpaces)
{
    std::string text = "     ";
    EXPECT_TRUE(isValidDocument(text));
}

/// нормальный запрос
TEST(UtilsTest, ValidRequestBasic)
{
    std::string request = "milk water";
    EXPECT_TRUE(isValidRequest(request));
}

// пустой запрос
TEST(UtilsTest, InvalidRequestEmpty)
{
    std::string request = "";
    EXPECT_FALSE(isValidRequest(request));
}

// запрос именно MAX_REQUEST_WORDS
TEST(UtilsTest, ValidRequestMAX_REQUEST_WORDS)
{
    std::string request;
    for (size_t i = 0; i < MAX_REQUEST_WORDS; ++i) {
        request += "milk ";
    }

    EXPECT_TRUE(isValidRequest(request));
}

// запрос именно больше чем MAX_REQUEST_WORDS
TEST(UtilsTest, InvalidRequestmoreMAX_REQUEST_WORDS)
{
    size_t toomachMAX_REQUEST_WORDS = MAX_REQUEST_WORDS +1;
    std::string request;
    for (size_t i = 0; i < toomachMAX_REQUEST_WORDS; ++i) {
        request += "milk ";
    }

    EXPECT_FALSE(isValidRequest(request));
}

//  запрос с заглавной буквой
TEST(UtilsTest, InvalidRequestUppercase)
{
    std::string request = "Milk water";
    EXPECT_FALSE(isValidRequest(request));
}

// запрос с цыфрой
TEST(UtilsTest, InvalidRequestDigit)
{
    std::string request = "milk1 water";
    EXPECT_FALSE(isValidRequest(request));
}

// запрос со знаком препинания
TEST(UtilsTest, InvalidRequestPunctuation)
{
    std::string request = "milk, water";
    EXPECT_FALSE(isValidRequest(request));
}

// запрос с несколькими пробелами между словами
TEST(UtilsTest, ValidRequestMultipleSpaces)
{
    std::string request = "milk    water";
    EXPECT_TRUE(isValidRequest(request));
}
