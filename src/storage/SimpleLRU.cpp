#include "SimpleLRU.h"
#include <iostream>

namespace Afina {
    namespace Backend {
        void SimpleLRU::delete_node(lru_node *cur_to_del) {
            _cur_size -= cur_to_del->value.size() + cur_to_del->key.size();
            auto prev_elem = cur_to_del->prev;
            prev_elem->next = std::move(cur_to_del->next);
            prev_elem->next->prev = prev_elem;
        }

        void SimpleLRU::add_node(const std::string &key, const std::string &value) {
            _cur_size += key.size() + value.size();
            std::unique_ptr<lru_node> item(new lru_node(key));
            item->value = value;
            item->next = std::move(_lru_tail->prev->next);
            item->prev = _lru_tail->prev;
            _lru_tail->prev = item.get();
            item->prev->next = std::move(item);
        }

        void SimpleLRU::put_anyway(const std::string &key, const std::string &value) {
            while (_max_size - _cur_size < key.size() + value.size()) {
                auto cur_to_del = _lru_head->next.get();
                _lru_index.erase(cur_to_del->key);
                delete_node(cur_to_del);
            }
            add_node(key, value);
            auto cur = _lru_tail->prev;
            _lru_index.insert({cur->key, *cur});
        }

        void SimpleLRU::move_to_tail(std::unique_ptr<lru_node> &cur_node, const std::string &value) {
            cur_node->value = value;
            cur_node->next = std::move(_lru_tail->prev->next);
            cur_node->prev = _lru_tail->prev;
            _lru_tail->prev = cur_node.get();
            cur_node->prev->next = std::move(cur_node);
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Put(const std::string &key, const std::string &value) {
            if (_max_size < key.size() + value.size()) return false;
            if (_lru_index.find(key) != _lru_index.end()) {
                return Set(key, value);
            }
            put_anyway(key, value);
            return true;
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
            if (_max_size < key.size() + value.size()) return false;
            if (_lru_index.find(key) != _lru_index.end()) return false;
            put_anyway(key, value);
            return true;
        }

      bool SimpleLRU::Set(const std::string &key, const std::string &value) {
          auto item = _lru_index.find(key);
          if (item == _lru_index.end()) return false;
          if (key.size() + value.size() > _max_size) return false;

          while (_max_size - _cur_size < key.size() + value.size()) {
              auto cur_to_del = _lru_head->next.get();
              _lru_index.erase(cur_to_del->key);
              delete_node(cur_to_del);
          }
          _cur_size += key.size()+value.size();

          std::unique_ptr<lru_node> cur_node = std::move(item->second.get().prev->next);
          auto cur_to_del = cur_node.get();
          delete_node(cur_to_del);
          move_to_tail(cur_node, value);
          return true;
      }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Delete(const std::string &key) {
            auto f = _lru_index.find(key);
            if(f == _lru_index.end()) return false;
            auto cur_to_del=&f->second.get();
            _lru_index.erase(f);
            delete_node(cur_to_del);
            return true;
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Get(const std::string &key, std::string &value) {
            auto item = _lru_index.find(key);
            if (item == _lru_index.end()) return false;

            std::unique_ptr<lru_node> cur_node = std::move(item->second.get().prev->next);
            auto cur_to_del = cur_node.get();
            delete_node(cur_to_del);

            value = item->second.get().value;
            _cur_size += key.size() + value.size();
            move_to_tail(cur_node, value);
            return true;
        }
    } // namespace Backend
} // namespace Afina