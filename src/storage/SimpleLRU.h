#include <utility>

#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <list>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage {
public:
    explicit SimpleLRU(size_t max_size = 1024) : _max_size(max_size), _free_size(_max_size) {}

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) override;

private:
    // LRU cache node
    using lru_node = struct lru_node {
        const std::string key;
        std::string value;

        lru_node(std::string key_i, std::string value_i) : key(std::move(key_i)), value(std::move(value_i)) {}
        // std::unique_ptr<lru_node> prev;
        // std::unique_ptr<lru_node> next;
    };

    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be less the _max_size
    std::size_t _max_size;

    std::size_t _free_size;

    // Main storage of lru_nodes, elements in this list ordered descending by "freshness": in the head
    // element that wasn't used for longest time.
    //
    // List owns all nodes
    std::list<lru_node> _lru_head;
    // lru_node* _lru_tail;

    // Index of nodes from list above, allows fast random access to elements by lru_node#key
    std::unordered_map<std::reference_wrapper<const std::string>, std::list<lru_node>::iterator, std::hash<std::string>, std::equal_to<const std::string>> _lru_index;
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
