#include <iostream>

#include "qt_impl/sdl_remote_adapter_qt_client.h"
#include "common/constants.h"
#include "rpc/detail/log.h"

namespace lua_lib {

RPCLIB_CREATE_LOG_CHANNEL(SDLRemoteTestAdapterQtClient)

namespace error_codes = constants::error_codes;

SDLRemoteTestAdapterQtClient::SDLRemoteTestAdapterQtClient(
            SDLRemoteTestAdapterClient* client_ptr
            ,TCPParams& in_params
            ,QObject* parent)
        : QObject(parent)
         ,tcp_params_(in_params)
{
    LOG_INFO("{0}",__func__);
    remote_adapter_client_ptr_ = client_ptr;
    try{
      future_ = exitSignal_.get_future();
    }catch (std::future_error& e) {
      std::cerr <<__func__<<" "<< e.what() <<"\n"<<std::flush;
    }
}

SDLRemoteTestAdapterQtClient::~SDLRemoteTestAdapterQtClient() {
  LOG_INFO("{0}",__func__);
  if(listener_ptr_){
    try{
      exitSignal_.set_value();
    }catch (std::future_error& e) {
      std::cerr <<__func__<<" "<< e.what() <<"\n"<<std::flush;
    }
    listener_ptr_->join();
  }
  
  if (isconnected_) {
    remote_adapter_client_ptr_->close(tcp_params_.host,tcp_params_.port);    
  }
}

void SDLRemoteTestAdapterQtClient::connect() {
  LOG_INFO("{0}",__func__);
  if (isconnected_ || listener_ptr_) {
    LOG_INFO("{0} Is already connected",__func__);
    int result = remote_adapter_client_ptr_->open(tcp_params_.host,tcp_params_.port);
    if (error_codes::SUCCESS == result) emit connected();
    return;
  }

  int result = remote_adapter_client_ptr_->open(tcp_params_.host,tcp_params_.port);    

  if (error_codes::SUCCESS == result) {
    try {
      isconnected_ = true;
      emit connected();   
      auto & future = future_;
      listener_ptr_.reset(new std::thread(
           [this,&future]
            {
                try{
                  
                  while (future.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout){
                    this->receive();
                  }
                
                }catch(std::future_error& e){
                  std::cerr <<"Exception in: "<<__func__<<" "<< e.what() <<"\n"<<std::flush;
                }catch(...){
                  std::cerr << "Unknown Exception in: " <<__func__<<"\n"<<std::flush;                  
                }
            }
          ));
    } catch (std::exception& e) {
      std::cerr <<__func__<<" "<< e.what() <<"\n"<<std::flush;
    }catch(...){
      std::cerr << "Unknown Exception in: " <<__func__<<"\n"<<std::flush;
    }
  }
}

int SDLRemoteTestAdapterQtClient::send(const std::string& data) {
  LOG_INFO("{0}",__func__);
  if (isconnected_) {
    using result = std::pair<std::string, int>;
    result res = remote_adapter_client_ptr_->send(tcp_params_.host,tcp_params_.port,data);
    if(error_codes::SUCCESS == res.second) {
      emit bytesWritten(data.length());
      if(res.first.length()){
        QString receivedData(res.first.c_str());
        emit textMessageReceived(receivedData);
      }
    }else if(error_codes::NO_CONNECTION == res.second) {
        connectionLost();
    }
    return res.second;
  }
  LOG_ERROR("{0}: Websocket was not connected",__func__);
  return error_codes::NO_CONNECTION;
}

std::pair<std::string, int> SDLRemoteTestAdapterQtClient::receive() {
  if (isconnected_) {
    std::pair<std::string, int> result =
          remote_adapter_client_ptr_->receive(tcp_params_.host,tcp_params_.port);
    if(error_codes::SUCCESS == result.second) {
      if(result.first.length()){
        QString receivedData(result.first.c_str());
        emit textMessageReceived(receivedData);
      }
    }else if(error_codes::NO_CONNECTION == result.second) {
        connectionLost();
    }
    return result;
  }
  LOG_ERROR("{0}: Websocket was not connected",__func__);
  return std::make_pair(std::string(),error_codes::NO_CONNECTION);
}

void SDLRemoteTestAdapterQtClient::connectionLost() {
  LOG_INFO("{0}",__func__);
  if (isconnected_) {
    isconnected_ = false;
    emit disconnected();
  }
}

} // namespace lua_lib
