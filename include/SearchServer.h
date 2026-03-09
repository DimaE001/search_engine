#ifndef SEARCHSERVER_H 
#define SEARCHSERVER_H

#include <vector>
#include <string>
#include <cstddef>

#include "InvertedIndex.h"
#include "constantsSearchServer.h"

struct RelativeIndex{
std::size_t doc_id;
float rank;

bool operator==(const RelativeIndex& other)const{
    return (doc_id == other.doc_id && rank == other.rank);
}

};


class SearchServer{
public:
/**
* @param idx в конструктор класса передаётся ссылка на класс InvertedIndex,
* чтобы SearchServer мог узнать частоту слов встречаемых в запросе
*/
SearchServer(InvertedIndex& idx, size_t max_responses = DEFAULT_MAX_RESPONSES)
        : _index(idx), _max_responses(max_responses) {}

/**
* Метод обработки поисковых запросов
* @param queries_input поисковые запросы взятые из файла requests.json
* @return возвращает отсортированный список релевантных ответов для заданных запросов
*/
std::vector<std::vector<RelativeIndex>> search(const std::vector<std::string>& queries_input);

private:
InvertedIndex& _index;
size_t _max_responses;

// возвращает вектор , редкость слов 
std::vector<std::string>  getQueryString(const std::string& query);

};

#endif