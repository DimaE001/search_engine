#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

// возвращает вектор запросов, внутри каждого запроса остаются только уникальные слова
std::vector<std::string> getUniqWords(const std::vector<std::string>& queries_input);

// верный документ для поиска
bool isValidDocument(const std::string& text);

// верный запрос для поиска
bool isValidRequest(const std::string& request);

#endif