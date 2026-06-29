#include "EcclesTypes.h"

ECCLES_API {

   e_boolean eccles_compareString(e_string a,e_string b){
    if(a == nullptr || b == nullptr) return false;
    while(*a == *b){
      if(*a == '\0') return true;
      ++a;
      ++b;
    }
    return false;
  }

  //runtime hashing function
  const e_uint32 eccles_hashRT(e_string s){
    e_uint32 h = 5381; //the hash modular
    if(s == nullptr) return h;
    
    while(*s){
      h = ((h << 5) + h) + (e_uint8)(*s);
      s++;
    }
    return h;
  }

  GLOBAL_STATE globalState;
};