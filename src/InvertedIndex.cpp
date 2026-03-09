#include <cstddef>
#include <vector>
#include <map>
#include <sstream>
#include <thread>
#include <string>
#include <atomic>
#include <algorithm>
#include <iostream>

#include "InvertedIndex.h"

InvertedIndex::InvertedIndex()
{
    threadsCount = std::thread::hardware_concurrency();

    if (threadsCount == 0) {
        // по умолчанию делаем 4 потока
        threadsCount = 4;
    }
}

void InvertedIndex::getLocalDictionary(std::atomic<size_t>& next_doc_id,
                                       std::map<std::string, std::vector<Entry>>& out) 
{
    out.clear();

    while (true) {
        size_t doc_id = next_doc_id.fetch_add(1);

        if (doc_id >= docs.size()) {
            break;
        }

        std::istringstream stream(docs[doc_id]);
        std::string word;

        while (stream >> word) {

            auto& entries = out[word];
            // поиск записи для этого doc_id
            auto it = std::find_if(entries.begin(), entries.end(),
                [doc_id](const Entry& e) {
                    return e.doc_id == doc_id;
                });

            if (it != entries.end()) {
                it->count++;                 // слово уже встречалось в этом документе
            } else {
                entries.push_back({doc_id, 1});  // новая запись
            }
        }
    }
}


void InvertedIndex::mergeDictionaries(const std::map<std::string, std::vector<Entry>>& local)
{
    // идём по словам локального словаря
    for (const auto& [word, localEntries] : local) {

        // получаем (или создаём) список Entry в общем индексе
        auto& globalEntries = freq_dictionary[word];

        // добавляем/суммируем все Entry из локального словаря
        for (const auto& le : localEntries) {

            // ищем, есть ли уже doc_id в глобальном списке
            auto it = std::find_if(globalEntries.begin(), globalEntries.end(),
                [&](const Entry& ge) {
                    return ge.doc_id == le.doc_id;
                });

            if (it != globalEntries.end()) {
                it->count += le.count;           // суммируем
            } else {
                globalEntries.push_back(le);     // добавляем новый doc_id
            }
        }
    }
}


void InvertedIndex::UpdateDocumentBase(const std::vector<std::string>& input_docs) {
    docs = input_docs;

    freq_dictionary.clear(); // очистить старый индекс

    // если документов нет - выходим сразу
    if (docs.empty()) {
        return;
    }

    // общий атомарный счётчик задач для всех потоков
    std::atomic<size_t> next_doc_id{0};

    // количество реально используемых потоков
    const size_t actualThreadsCount = std::min(threadsCount, docs.size());

    // локальные словари для каждого потока
    std::vector<std::map<std::string, std::vector<Entry>>> locals(actualThreadsCount);

    // создаём и запускаем потоки
    std::vector<std::thread> threads;
    threads.reserve(actualThreadsCount);

    for (size_t i = 0; i < actualThreadsCount; ++i) {
        threads.emplace_back([this, &next_doc_id, &locals, i]() {
            // каждый поток заполняет только свой locals[i]
            this->getLocalDictionary(next_doc_id, locals[i]);
        });
    }

    // дожидаемся завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }

    // сливаем все локальные словари в общий индекс
    for (const auto& local : locals) {
        mergeDictionaries(local);
    }
}

/**
* Метод определяет количество вхождений слова word в загруженной базе документов
* @param word слово, частоту вхождений которого не обходимо определить
* @return возвращает подготовленный список с частотой слов
*/
std::vector<Entry> InvertedIndex::GetWordCount(const std::string& word) const {

    auto it = freq_dictionary.find(word);
        if (it == freq_dictionary.end()) {
            return {};
        }
    return it->second;
}

void InvertedIndex::printIndex() const {

    for (const auto& [word, entries] : freq_dictionary) {

        std::cout << word << " : \t\t";

        for (const auto& e : entries) {
            std::cout << "(" << e.doc_id
                      << "," << e.count << ") ";
        }

        std::cout << std::endl;
    }
}