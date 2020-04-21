#include "Connection.h"

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

namespace Afina {
namespace Network {
namespace MTnonblock {

// See Connection.h
void Connection::Start() { std::cout << "Start" << std::endl; }

// See Connection.h
void Connection::OnError() {
    std::cout << "OnError" << std::endl;
    _is_alive = false;
}

// See Connection.h
void Connection::OnClose() {
    std::cout << "OnClose" << std::endl;
    _is_alive = false;
}

// See Connection.h
void Connection::DoRead(std::shared_ptr<Afina::Storage> pStorage) {
    while(int read_tmp = read(_socket,_in_buffer + _offset,_max_size - _offset)){
        if(read_tmp == 0){
            OnClose();
            return;
        }
        if(read_tmp == -1){
            if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
                OnError();
            }
            return;
        }
        read_tmp += _offset;
        _offset = 0;
        while(read_tmp > 0){
            if(!_command_to_execute){
                size_t parsed;
                if(_parser.Parse(_in_buffer, read_tmp, parsed)){
                    _command_to_execute=_parser.Build(_arg_remains);

                    if (_arg_remains > 0) {
                        _arg_remains += 2;
                    }
                }
                if(parsed) {
                    read_tmp -= parsed;
                    std::memmove(_in_buffer, _in_buffer + parsed, read_tmp);
                } else{
                    _offset = read_tmp;
                    break;
                }
            }
            if(_command_to_execute && _arg_remains > 0){
                auto len = std::min(int(_arg_remains), read_tmp);
                _argument_for_command.append(_in_buffer, len);
                read_tmp -= len;
                _arg_remains -= len;
                std::memmove(_in_buffer,_in_buffer + len, read_tmp);
            }
            if(_command_to_execute && !_arg_remains) {
                std::string res;
                _command_to_execute->Execute(*pStorage, _argument_for_command, res);

                if (_written == 0) {
                    _event.events |= EPOLLOUT;
                }
                std::memcpy(_out_buffer + _written, res.data(),res.size());
                _written += res.size();
                if(_written + 100 > _max_size) {
                    _event.events = EPOLLOUT;
                }
                _parser.Reset();
                _command_to_execute.reset();
                _argument_for_command.clear();

            }
        }
    }
}

// See Connection.h
void Connection::DoWrite() {
    int sent = send(_socket,_out_buffer,_written,0);

    if(sent == 0){
        OnClose();
        return;
    }
    if(sent == -1) {
        OnError();
        return;
    }
    _written -= sent;
    std::memmove(_in_buffer,_in_buffer + sent,_written);
    if(_written < _max_size - 100) {
        _event.events = EPOLLOUT + EPOLLIN;
    }
    if(_written == 0) {
        _event.events = EPOLLIN;
    }
}

} // namespace MTnonblock
} // namespace Network
} // namespace Afina
