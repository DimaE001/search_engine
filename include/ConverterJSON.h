#ifndef CONVERTERJSON_H 
#define CONVERTERJSON_H


#include <cstddef>
#include <string>
#include <vector>

#include "SearchServer.h"
#include "constantsSearchServer.h"


class ConverterJSON {
private:
    // Документы в которых ведется поиск.
    std::vector<std::string> textDocuments_;
    // Максимальное значение количества выдачи в result.json, значение поумолчанию DEFAULT_MAX_RESPONSES
    int maxResponses_ = DEFAULT_MAX_RESPONSES;
      

public:
    ConverterJSON()=default;
    
    // Метод получения из config.json настроек и проверки настроек.
    void getConfigJson(const std::string& path); 
   
    // Метод получения содержимого файлов
    // @return Возвращает список с содержимым файлов перечисленных
    // в config.json
    std::vector<std::string> getTextDocuments();

    // Метод считывает поле max_responses для определения предельного
    // количества ответов на один запрос
    // @return
    size_t getResponsesLimit();

    // Метод получения запросов из файла requests.json
    // @return возвращает список запросов  из файла requests.json
    std::vector<std::string> getRequests(const std::string requestsPath_);

    // Метод получения результатов поиска в answers.json
    // результаты поисковых запросов
    void putAnswers(const std::vector<std::vector<RelativeIndex>>& answers, std::string answersPath_);

};

#endif