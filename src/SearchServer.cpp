#include <sstream>
#include <vector>
#include <map>
#include <cctype>
#include <algorithm>

#include "SearchServer.h"
#include "utilsSearchServer.h"

std::vector<std::string> SearchServer::getQueryString(const std::string& query){
    // возвращаем просто слова в нужном порядке
    std::vector<std::string> result;

    std::istringstream stream(query);
    std::string word;
    
    // для уникальности слов в запросе
    std::map<std::string, size_t> seen; 

    // временная группировка по редкости
    std::map<size_t, std::vector<std::string>> by_rarity;

    while (stream >> word) {
    
        // проверяем уникальность
        if (seen.find(word) != seen.end()) {
            continue;
        }
        seen[word] = 1;

    
        // редкость = суммарная частота слова во всех документах
        auto entries = _index.GetWordCount(word);

        size_t total_count = 0;
        for (const auto& entry : entries) {
            total_count += entry.count;
        }

        by_rarity[total_count].push_back(word);
    }

    // расплющиваем map в vector в порядке ключей, редкие впереди
    for (const auto& [rarity, words] : by_rarity) {
        for (const auto& w : words) {
            result.push_back(w);
        }
    }

    return result;
}

/**
* Метод обработки поисковых запросов
* @param queries_input поисковые запросы взятые из файла requests.json
* @return возвращает отсортированный список релевантных ответов для заданных запросов
*/
std::vector<std::vector<RelativeIndex>> SearchServer::search(const std::vector<std::string>& queries_input)
{
    std::vector<std::vector<RelativeIndex>> result;

    // нормализуем и делаем уникальные слова в каждом запросе
    std::vector<std::string> queries_uniq = getUniqWords(queries_input);

    for (const auto& query : queries_uniq) {

        // doc_id -> суммарная частота слов запроса для документов, содержащих все слова
        std::map<size_t, size_t> doc_freq;

        // слова запроса отсортированы по редкости редкие -> частые
        auto words_sorted = getQueryString(query);

        // пустой запрос
        if (words_sorted.empty()) {
            result.push_back({});
            continue;
        }

        bool first_word = true;

        // AND пересечение кандидатов
        for (const auto& word : words_sorted) {

            auto entries = _index.GetWordCount(word);

            // слово не найдено ни в одном документе -> AND невозможен -> пустой ответ
            if (entries.empty()) {
                doc_freq.clear();
                break;
            }

            if (first_word) {
                // инициализация кандидатов по первому слову
                for (const auto& entry : entries) {
                    doc_freq[entry.doc_id] = entry.count;
                }
                first_word = false;
            } else {
                // пересечение кандидатов со следующим словом
                std::map<size_t, size_t> new_doc_freq;

                for (const auto& entry : entries) {
                    auto it = doc_freq.find(entry.doc_id);
                    if (it != doc_freq.end()) {
                        new_doc_freq[entry.doc_id] = it->second + entry.count;
                    }
                }

                doc_freq = std::move(new_doc_freq);

                // если кандидатов не осталось дальше смысла нет
                if (doc_freq.empty()) {
                    break;
                }
            }
        }

        // ничего не найдено
        if (doc_freq.empty()) {
            result.push_back({});
            continue;
        }

        // найти максимум для нормализации rank
        size_t max_count = 0;
        for (const auto& [doc_id, count] : doc_freq) {
            if (count > max_count) {
                max_count = count;
            }
        }

        // сформировать результат для этого запроса
        std::vector<RelativeIndex> query_result;
        query_result.reserve(doc_freq.size());

        for (const auto& [doc_id, count] : doc_freq) {
            query_result.push_back(RelativeIndex{
                doc_id,
                static_cast<float>(count) / static_cast<float>(max_count)
            });
        }

        // сортировка по rank убыванию, при равенстве - doc_id возрастанию
        std::sort(query_result.begin(), query_result.end(),
            [](const RelativeIndex& a, const RelativeIndex& b) {
                if (a.rank == b.rank) {
                    return a.doc_id < b.doc_id;
                }
                return a.rank > b.rank;
            });
        
            
            if (query_result.size() > _max_responses) 
                query_result.resize(_max_responses);
                
        result.push_back(std::move(query_result));
    }

    return result;
}