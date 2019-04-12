#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    auto desired_size = key.size() + value.size();
    if (desired_size > _max_size) {
        return false;
    }

    Delete(key);

    while (_free_size < desired_size) {
        Delete(_lru_head.begin()->key);
    }

    lru_node new_node(key, value);

    _lru_head.push_back(std::move(new_node));
    _lru_index[new_node.key] = std::prev(_lru_head.end());
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    if (_lru_index.find(key) == _lru_index.end()) {
        return Put(key, value);
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    auto elem_to_update = _lru_index.find(key);
    if (elem_to_update == _lru_index.end()) {
        return false;
    }

    auto desired_size = key.size() + value.size();
    if (desired_size > _max_size) {
        return false;
    }

    while (_free_size < desired_size) {
        if (_lru_head.begin() == elem_to_update->second)
            continue;
        Delete(_lru_head.begin()->key);
    }

    elem_to_update->second->value = value;
    _lru_head.splice(_lru_head.end(), _lru_head, elem_to_update->second);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto elem_to_delete = _lru_index.find(key);

    if (elem_to_delete == _lru_index.end()) {
        return false;
    }

    auto size_of_elem = elem_to_delete->second->key.size() + elem_to_delete->second->value.size();
    _free_size += size_of_elem;

    _lru_head.erase(elem_to_delete->second);
    _lru_index.erase(elem_to_delete);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    auto elem_to_get = _lru_index.find(key);
    if (elem_to_get == _lru_index.end()) {
        return false;
    }

    value = elem_to_get->second->value;
    return true;
}
} // namespace Backend
} // namespace Afina
