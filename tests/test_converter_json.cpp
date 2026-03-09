 #include "gtest/gtest.h"
#include <cstddef>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include "ConverterJSON.h"

// просто считали файл конфиг
TEST(ConverterJSONTest, getConfigJsonBasic)
{
    std::ofstream file("test_config.json");
    file << R"({
        "config": {
            "name": ")" << ENGINE_LABEL << R"(",
            "version": ")" << ENGINE_VERSION << R"(",
            "max_responses": )" << DEFAULT_MAX_RESPONSES << R"(
        },
        "files": []
    })";
    file.close();

    ConverterJSON converter;

    std::string path {"test_config.json"};
    EXPECT_NO_THROW(
        converter.getConfigJson(path)
    );

    std::remove("test_config.json");
}

//нет такого конфига
TEST(ConverterJSONTest, getConfigJsonNoFile)
{
    ConverterJSON converter;
    std::remove("nofile.json");
    std::string path_no_file {"nofile.json"};

    EXPECT_THROW(
        converter.getConfigJson(path_no_file),
        std::runtime_error
    );
}

//список доков пустой
TEST(ConverterJSONTest, getConfigJsonEmptyConfig)
{
    std::remove("test_config_e.json");

    std::ofstream file("test_config_e.json");
    file << R"({
        "files": []
    })";
    file.close();

    ConverterJSON converter;
    std::string path{"test_config_e.json"};

    EXPECT_THROW(
        converter.getConfigJson(path),
        std::invalid_argument
    );

    std::remove("test_config_e.json");
}

// нет files в конфиге
TEST(ConverterJSONTest, getConfigJsonMissingFilesArray)
{
    std::remove("test_config_m.json");
    std::ofstream file("test_config_m.json");
    file << R"({
        "config": {
            "name": ")" << ENGINE_LABEL << R"(",
            "version": ")" << ENGINE_VERSION << R"(",
            "max_responses": )" << DEFAULT_MAX_RESPONSES << R"(
        }
    })";
    file.close();

    ConverterJSON converter;
    std::string path{"test_config_m.json"};

    EXPECT_THROW(
        converter.getConfigJson(path),
        std::invalid_argument
    );

    std::remove("test_config_m.json");
}

//не та версия
TEST(ConverterJSONTest, getConfigJsonErrorVersion)
{
    std::string wrong_version = ENGINE_VERSION + "1";
    std::remove("test_config_w.json");
    std::ofstream file("test_config_w.json");
    file << R"({
        "config": {
            "name": ")" << ENGINE_LABEL << R"(",
            "version": ")" << wrong_version << R"(",
            "max_responses": )" << DEFAULT_MAX_RESPONSES << R"(
        },
        "files": []
    })";
    file.close();

    ConverterJSON converter;
    std::string path{"test_config_w.json"};

    EXPECT_THROW(
        converter.getConfigJson(path),
        std::runtime_error
    );

    std::remove("test_config_w.json");
}

//нет версии
TEST(ConverterJSONTest, getConfigJsonNoVersion)
{
    std::remove("test_config_w.json");
    std::ofstream file("test_config_w.json");
    file << R"({
        "config": {
            "name": ")" << ENGINE_LABEL << R"(",
            "max_responses": )" << DEFAULT_MAX_RESPONSES << R"(
        },
        "files": []
    })";
    file.close();

    ConverterJSON converter;
    std::string path{"test_config_w.json"};

    EXPECT_THROW(
        converter.getConfigJson(path),
        std::invalid_argument
    );

    std::remove("test_config_w.json");
}

//выдача задана 10
TEST(ConverterJSONTest, getConfigJsonNewMaxResponce)
{
    int new_max = 10;
    std::ofstream file("test_config_mm.json");
    file << R"({
        "config": {
            "name": ")" << ENGINE_LABEL << R"(",
            "version": ")" << ENGINE_VERSION << R"(",
            "max_responses": )" << new_max << R"(
    },
    "files": []
    })";
    file.close();

    ConverterJSON converter;
    std::string path {"test_config_mm.json"};
    converter.getConfigJson(path);

    EXPECT_EQ(
    converter.getResponsesLimit(),
    new_max
    );

    std::remove("test_config_mm.json");
}

// выдача size_t
TEST(ConverterJSONTest, getConfigJsonWrongMaxResponsesType)
{
    std::ofstream file("test_config_t.json");
    file << R"({
        "config": {
            "name": ")" << ENGINE_LABEL << R"(",
            "version": ")" << ENGINE_VERSION << R"(",
            "max_responses": "ten"
        },
        "files": []
    })";
    file.close();

    ConverterJSON converter;
    std::string path{"test_config_t.json"};

    EXPECT_THROW(
        converter.getConfigJson(path),
        std::invalid_argument
    );

    std::remove("test_config_t.json");
}

//выдача не задана
TEST(ConverterJSONTest, getConfigJsonNoMaxResponce)
{
    std::ofstream file("test_config_n.json");
    file << R"({
        "config": {
            "name": ")" << ENGINE_LABEL << R"(",
            "version": ")" << ENGINE_VERSION << R"("
    },
    "files": []
    })";
    file.close();

    ConverterJSON converter;
    std::string path {"test_config_n.json"};
    converter.getConfigJson(path);

    EXPECT_EQ(
    converter.getResponsesLimit(),
    DEFAULT_MAX_RESPONSES
    );

    std::remove("test_config_n.json");
}


//файл с запросами норм
TEST(ConverterJSONTest, ReadRequests)
{
    std::ofstream file("test_requests.json");
    file << R"({
        "requests": [
            "milk water",
            "apple juice"
        ]
    })";
    file.close();

    ConverterJSON converter;
    auto result = converter.getRequests("test_requests.json");

    std::vector<std::string> expected = {
        "milk water",
        "apple juice"
    };

    EXPECT_EQ(result, expected);

    std::remove("test_requests.json");
}

// запросы должгы быть string
TEST(ConverterJSONTest, getRequestsElementIsNotString)
{
    std::ofstream file("test_requests_s.json");
    file << R"({
        "requests": [
            "milk water",
            123
        ]
    })";
    file.close();

    ConverterJSON converter;
    std::string path{"test_requests_s.json"};

    EXPECT_THROW(
        converter.getRequests(path),
        std::invalid_argument
    );

    std::remove("test_requests_s.json");
}

//файл с запросами отсутствует
TEST(ConverterJSONTest, NoFileRequests)
{
    ConverterJSON converter;
    std::remove("no_requests_f.json");
    std::string path_no_file {"no_requests_f.json"};

    EXPECT_THROW(
        converter.getRequests(path_no_file),
        std::runtime_error
    );
}

//файл с ответами записан корректно
TEST(ConverterJSONTest, putAnswersBasic)
{
    ConverterJSON converter;

    std::string path{"test_answers_g.json"};

    const std::vector<std::vector<RelativeIndex>> answers = {
        { {2,1.0}, {1,0.8}, {0,0.2} },
        { {2,1.0} },
        {},
        { {2,1.0} }
    };

    converter.putAnswers(answers, path);

    std::ifstream file(path);
    ASSERT_TRUE(file.is_open());

    nlohmann::json result;
    file >> result;

    EXPECT_TRUE(result.contains("answers"));

    EXPECT_TRUE(result["answers"]["request001"]["result"]);
    EXPECT_FALSE(result["answers"]["request003"]["result"]);

    EXPECT_EQ(result["answers"]["request001"]["relevance"][0]["docid"],2);

    EXPECT_FALSE(result["answers"]["request003"].contains("relevance"));

    file.close();
    std::remove(path.c_str());
}

// если файл есть, перезаписываем
TEST(ConverterJSONTest, putAnswersOverwriteFile)
{
    ConverterJSON converter;

    std::string path{"test_answers_o.json"};

    // создаём файл с мусором
    std::ofstream file(path);
    file << "old data";
    file.close();

    const std::vector<std::vector<RelativeIndex>> answers = {
        { {1, 1.1} }
    };

    converter.putAnswers(answers, path);

    std::ifstream in(path);
    ASSERT_TRUE(in.is_open());

    nlohmann::json result;
    in >> result;

    EXPECT_TRUE(result.contains("answers"));
    EXPECT_EQ(result["answers"]["request001"]["result"], true);

    in.close();
    std::remove(path.c_str());
}

// файлы доблжны быть строкой
TEST(ConverterJSONTest, ConfigFilePathMustBeString)
{
    std::ofstream file("test_config_ss.json");
    file << R"({
        "config": {
            "name": ")" << ENGINE_LABEL << R"(",
            "version": ")" << ENGINE_VERSION << R"("
        },
        "files": [123]
    })";
    file.close();

    ConverterJSON converter;

    std::string path{"test_config_ss.json"};

    EXPECT_THROW(
        converter.getConfigJson(path),
        std::invalid_argument
    );

    std::remove(path.c_str());
}

// много запросов
TEST(ConverterJSONTest, getRequestsTooManyRequests)
{
    std::ofstream file("test_requests_t.json");
    file << R"({"requests":[)";
    size_t to_mach_MAX_REQUESTS = MAX_REQUESTS + 1;
    for (size_t i = 0; i < to_mach_MAX_REQUESTS; ++i) {
        if (i > 0) file << ",";
        file << "\"milk\"";
    }

    file << "]}";
    file.close();

    ConverterJSON converter;
    std::string path{"test_requests_t.json"};

    EXPECT_THROW(
        converter.getRequests(path),
        std::invalid_argument
    );

    std::remove("test_requests_t.json");
}

// норм запросов
TEST(ConverterJSONTest, getRequestsMAX_REQUESTS)
{
    std::ofstream file("test_requests_mr.json");
    file << R"({"requests":[)";

    for (int i = 0; i < MAX_REQUESTS; ++i) {
        if (i > 0) file << ",";
        file << "\"word\"";
    }

    file << "]}";
    file.close();

    ConverterJSON converter;
    std::string path{"test_requests_mr.json"};

    auto result = converter.getRequests(path);

    EXPECT_EQ(result.size(), MAX_REQUESTS);

    std::remove("test_requests_mr.json");
}

// запрос верный
TEST(ConverterJSONTest, getRequestsInvalidFormat)
{
    std::string path{"test_requests.json"};
    std::ofstream file(path);
    file << R"({
        "requests": [
            "milk water",
            "Milk"
        ]
    })";
    file.close();

    ConverterJSON converter;
    auto result = converter.getRequests(path);

    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], "milk water");
    EXPECT_EQ(result[1], "");

    std::remove(path.c_str());
}