#pragma once

#include "key_value_db.hh"

namespace fil {

  class kv_rocksdb {

  public:
    struct initializer_type {

      std::string connection_url;
      std::vector<key_value> initial_kv;

    };

    
  private:
    
    
  }
  
}
