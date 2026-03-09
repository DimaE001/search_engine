#ifndef INVERTEDINDEX_H 
#define INVERTEDINDEX_H

#include <cstddef>
#include <map>
#include <vector>
#include <string>
#include <atomic>

struct Entry{
size_t doc_id;
size_t count;

// Данный оператор необходим для проведения тестовых сценариев
    bool operator==(const Entry& other)const{
        return(doc_id == other.doc_id && count == other.count);
    }
};


class InvertedIndex{
public:
InvertedIndex();

void printIndex() const;
/**
* Обновить или заполнить базу документов, по которой будем совершать поиск
* @param texts_input содержимое документов
*/
void UpdateDocumentBase(const std::vector<std::string>& input_docs);
/**


* Метод определяет количество вхождений слова word в
    загруженной базе документов
* @param word слово, частоту вхождений которого необходимо определить
* @return возвращает подготовленный список с частотой слов
*/
std::vector<Entry> GetWordCount(const std::string& word) const;

private:
// количество потоков для обработки документов
size_t threadsCount;

// список содержимого документов
std::vector<std::string> docs;

//частотный словарь word в список {doc_id, count}
std::map<std::string,std::vector<Entry>> freq_dictionary;

// парралельно считывает индекс для документа.
void getLocalDictionary(std::atomic<size_t>& next_doc_id,
                        std::map<std::string, std::vector<Entry>>& out);

//объединяем потоки
void mergeDictionaries(const std::map<std::string, std::vector<Entry>>& local);

};

#endif