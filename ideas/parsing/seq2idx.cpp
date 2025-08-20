#include <iostream>
#include <vector>
#include <string>
#include <optional>

using namespace std;

struct prefix_store{
 using c_t = char;
 using i_t = size_t;
 struct link{
  enum class type {value,ref};
  type type;
  c_t value;
  i_t ref;
  i_t next;
 };
 vector<link> mem;
 prefix_store(){
  mem.push_back(link{}); // the empty string is mapped to 0, hence the start of the real data needs to start after thst
 }
 optional<i_t> match(string, bool);
};

optional<prefix_store::i_t> prefix_store::match(string s, bool insert){
 i_t matched{};
 i_t mem_pos{1};

 for(; mem_pos < mem.size() && matched < s.length(); ++mem_pos){
  if(mem[mem_pos].type == link::type::value){
  } else if (mem[mem_pos].type == link::type::ref){
  }
 }
 if (matched == s.length()) return mem_pos - 1;
 return {};
}

void test(){
 prefix_store prefix_store;
 auto r1 = prefix_store.match("", true);
 auto r2 = prefix_store.match("Hello, World!", true);
 if (r1) cout << *r1 << '\n'; else cout << "failed\n";
 if (r2) cout << *r2 << '\n'; else cout << "failed\n"; 
}

int main(int argc, char** argv){
 test();
 return 0;
}
