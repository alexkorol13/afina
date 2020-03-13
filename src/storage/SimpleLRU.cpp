#include "SimpleLRU.h"
#include <iostream>

namespace Afina {
    namespace Backend {
// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Put(const std::string &key, const std::string &value) {
            if (_max_size < key.size() + value.size()) return false;
            std::unique_ptr<lru_node> cur_node;
            auto this_elem = _lru_index.find(key);
            bool item_exists = (this_elem != _lru_index.end());
            if (item_exists) {
                auto tmp = &this_elem->second.get();
                cur_node = std::move(tmp->prev->next);

                //delete item
                _cur_size -= tmp->value.size() + tmp->key.size();
                auto prev_elem = tmp->prev;
                prev_elem->next = std::move(tmp->next);
                prev_elem->next->prev = prev_elem;
            }
            while (true) {
                if (_max_size - _cur_size >= key.size() + value.size()) break;
                auto cur_to_del = _lru_head->next.get();
                _lru_index.erase(cur_to_del->key);

                //delete item
                _cur_size -= cur_to_del->value.size() + cur_to_del->key.size();
                auto prev_elem = cur_to_del->prev;
                prev_elem->next = std::move(cur_to_del->next);
                prev_elem->next->prev = prev_elem;
            }
            if (item_exists) {
                auto ptr = std::move(cur_node);
                ptr->value = value;
                _cur_size += value.size() + key.size();
                ptr->next = std::move(_lru_tail->prev->next);
                ptr->prev = _lru_tail->prev;
                _lru_tail->prev = ptr.get();
                ptr->prev->next = std::move(ptr);
            } else {

                //add item
                _cur_size += key.size() + value.size();
                std::unique_ptr<lru_node> item(new lru_node(key));
                item->value = value;
                item->next = std::move(_lru_tail->prev->next);
                item->prev = _lru_tail->prev;
                _lru_tail->prev = item.get();
                item->prev->next = std::move(item);

                auto cur = _lru_tail->prev;
                _lru_index.insert({cur->key, *cur});
            }
            return true;
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
            if (_max_size < key.size() + value.size()) return false;
            std::unique_ptr<lru_node> cur_node;
            auto this_elem = _lru_index.find(key);
            bool item_exists = (this_elem != _lru_index.end());
            if (item_exists) return false;
            while (true) {
                if (_max_size - _cur_size >= key.size() + value.size()) break;
                auto cur_to_del = _lru_head->next.get();
                _lru_index.erase(cur_to_del->key);

                //delete item
                _cur_size -= cur_to_del->value.size() + cur_to_del->key.size();
                auto prev_elem = cur_to_del->prev;
                prev_elem->next = std::move(cur_to_del->next);
                prev_elem->next->prev = prev_elem;
            }
            //add item
            _cur_size += key.size() + value.size();
            std::unique_ptr<lru_node> item(new lru_node(key));
            item->value = value;
            item->next = std::move(_lru_tail->prev->next);
            item->prev = _lru_tail->prev;
            _lru_tail->prev = item.get();
            item->prev->next = std::move(item);

            auto cur = _lru_tail->prev;
            _lru_index.insert({cur->key, *cur});
            return true;
        }

      bool SimpleLRU::Set(const std::string &key, const std::string &value) {
          auto item = _lru_index.find(key);
          if (item == _lru_index.end()) return false;
          if (_cur_size - item->second.get().value.size() + value.size() > _max_size) return false;
          _cur_size += key.size()+value.size();
          std::unique_ptr<lru_node> cur_node = std::move(item->second.get().prev->next);

          //delete item
          auto cur_to_del = cur_node.get();
          _cur_size -= cur_to_del->value.size() + cur_to_del->key.size();
          auto prev_elem = cur_to_del->prev;
          prev_elem->next = std::move(cur_to_del->next);
          prev_elem->next->prev = prev_elem;

          //connect
          cur_node->value = value;
          cur_node->next = std::move(_lru_tail->prev->next);
          cur_node->prev = _lru_tail->prev;
          _lru_tail->prev = cur_node.get();
          cur_node->prev->next = std::move(cur_node);
          return true;
      }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Delete(const std::string &key) {
            auto f = _lru_index.find(key);
            if(f == _lru_index.end()) return false;

            auto cur_to_del=&f->second.get();
            _lru_index.erase(f);

            //delete item
            _cur_size -= cur_to_del->value.size() + cur_to_del->key.size();
            auto prev_elem = cur_to_del->prev;
            prev_elem->next = std::move(cur_to_del->next);
            prev_elem->next->prev = prev_elem;

            return true;
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Get(const std::string &key, std::string &value) {
            auto item = _lru_index.find(key);
            if (item == _lru_index.end()) return false;

            std::unique_ptr<lru_node> cur_node = std::move(item->second.get().prev->next);
            auto cur_to_del = cur_node.get();

            //delete item
            _cur_size -= cur_to_del->value.size() + cur_to_del->key.size();
            auto prev_elem = cur_to_del->prev;
            prev_elem->next = std::move(cur_to_del->next);
            prev_elem->next->prev = prev_elem;

            //connect
            value = item->second.get().value;
            _cur_size += key.size() + value.size();
            cur_node->value = value;
            cur_node->next = std::move(_lru_tail->prev->next);
            cur_node->prev = _lru_tail->prev;
            _lru_tail->prev = cur_node.get();
            cur_node->prev->next = std::move(cur_node);
            return true;
        }
    } // namespace Backend
} // namespace Afina