#ifndef CONSTANTSSEARCHSERVER_H
#define CONSTANTSSEARCHSERVER_H

#include <cstddef>
#include <string>

// имена файлов конфигурация, запросы, выдача
inline const std::string CONFIG_FILE = "config.json";
inline const std::string REQUESTS_FILE = "requests.json";
inline const std::string ANSWERS_FILE = "answers.json";

// параметры движка
inline const std::string ENGINE_LABEL = "SkillboxSearchEngine";
inline const std::string ENGINE_VERSION = "1.1";

// ограничения
constexpr size_t DEFAULT_MAX_RESPONSES = 5;
constexpr size_t MAX_REQUESTS = 1000;

constexpr size_t MAX_DOCUMENT_WORDS = 1000;
constexpr size_t MAX_DOCUMENT_WORD_LENGTH = 100;

constexpr size_t MAX_REQUEST_WORDS = 10;

#endif