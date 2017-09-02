#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <cstdint>
#include <memory>
#include <iostream>
#include <sys/stat.h>
#include <locale>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <vector>
#include <climits>
#include <map>


/*
 *
 *
 *
 *
 *
 *
 *
 *    preprocess(T)
 *
 *    start:
 *      if (buffer = [t_1,...,t_n] is not empty)
 *        extract t_1 from buffer T = t_1; return true
 *      r = get_raw_token(T)
 *      if not r => return false
 *      check for matching rewrite rules
 *      take the rewrite rule with maximal index, if there is none => return true
 *      buffer = [t_1,t_2,...,t_n] where t_i is the ith token of the right hand side of the chosen rule
 *      goto start
 *
 *
 *    get_token(T)
 *      r = preprocess(T)
 *      if not r => return false
 *      for all blocks B (most recently defined ones first)
 *       if precond(B) => deactivate all rewrite rules and
 *                        activate all rewrite rules of B
 *                        return preprocess(T)
 *      return true
 *
 *
 *
 *
 *
 *    Lexer Rewrite Script (CANOps example)
 *    %%
 *
 *    DEFAULT{
 *      ws =>
 *      else => error("Unexpected token.")
 *    }
 *    BEGIN{
 *     VERSION string => version { $2; };
 *     NS_ : => new_symbols {    / $ns_active=1
 *     ident |$ns_active&&suffix($1)!="_" => $1 ;
 *     ident |$ns_active&&suffix($1)=="_" => }; $1 /$ns_active=0
 *     eof |ns_active => }; eof
 *    }
 *
 *    BU_{
 *     BU_ : => units{
 *     ident|suffix($1)!="_" => $1 ;
 *     ident|suffix($1)=="_" => }; $1
 *     eof => }; eof
 *    }
 *
 *    VAL_TABLE_{
 *      VAL_TABLE_ ident => value_table{
 *      int string => pair{index{$1;};value{$2;};};
 *      ident|suffix($1)=="_" => }; $1 / exit()
 *      eof => }; eof
 *    }
 *
 *
 *
 *
 *
 *
 * */


template<typename T> struct nonmutable_string
{
 using size_type = std::size_t;
 using ptr_t = const T*;

 const T* ptr_;
 size_type size_;

 size_type length() const {return size_;}
 T operator [] (size_type i) const {return ptr_[i];}
 ptr_t & ptr() {return ptr_;}
 size_type & size(){return size_;}
 size_type  size() const {return size_;}

 nonmutable_string() = default;
 nonmutable_string(const T* p,size_type s):ptr_(p),size_(s){}
 const T* data() const {return ptr_;}
};

bool operator == (nonmutable_string<char> const &  lhs ,  nonmutable_string<char> const &  rhs);
bool operator != (nonmutable_string<char> const &  lhs ,  nonmutable_string<char> const &  rhs);
bool operator < (nonmutable_string<char> const &  lhs ,  nonmutable_string<char> const &  rhs);
bool operator == (nonmutable_string<char> const &  lhs ,  const char * rhs);
bool operator != (nonmutable_string<char> const &  lhs , const char * rhs);
std::ostream& operator << (std::ostream & o, nonmutable_string<char> const & s);

template<typename T>struct Memory
{
  Memory() = default;
  Memory(Memory const & rhs):ptr_(rhs.ptr_), size_(rhs.size_), pos_(rhs.pos_),owner_(false)
  {
  }
  Memory(Memory && rhs) : ptr_(rhs.ptr_), size_(rhs.size_), pos_(rhs.pos_),owner_(true)
  {
	  rhs.ptr_ = nullptr;
  }

  Memory& operator = (Memory const & rhs)
  {
	  ptr_  = rhs.ptr_;
	  size_ = rhs.size_;
	  pos_ = rhs.pos_;
	  owner_ = false;
	  return *this;
  }

  T * ptr_;
  std::intmax_t size_,pos_;
  bool owner_;

  std::intmax_t size() const {return size_;}
  std::intmax_t pos() const {return pos_;}
  std::intmax_t& pos() {return pos_;}

  typedef std::pair<std::intmax_t, std::intmax_t> state_t;

  std::pair<std::intmax_t, std::intmax_t> get_state() const
  {
    return  {size_,pos_};
  }

  void read_state(std::pair<std::intmax_t, std::intmax_t> s)
  {
    size_ = s.first;
    pos_  = s.second;
  }

  void unget()
  {
    if(pos_ == 0) return;
    --pos_;
    if(ptr_[pos_] == '\n' || ptr_[pos_] == '\r') {
      for(;pos_ > 0 && (ptr_[pos_] == '\n' || ptr_[pos_] == '\r');--pos_);
      if(ptr_[pos_] != '\n' && ptr_[pos_] != '\r') ++pos_;
    }
  }

  bool get(T& ch)
   {

    if (pos_ >= size_) return false;
    auto pos = pos_;
    for(;ptr_[pos] == '\n' || ptr_[pos] == '\r';++pos) if (pos >= size_) break;
    if (pos_ < pos) {ch = '\n';pos_ = pos; return true;}
    ch = ptr_[pos_];++pos_;

    return true;
   }

   bool next_equals(T const & ch)
   {
    if (pos_ >= size_) return false;
    if ( ptr_[pos_] == '\r' ) return ch == '\r' || ch == '\n';
    return ch == ptr_[pos_];
   }

   T next() {
    if (pos_ >= size_) return T{};
    return ptr_[pos_];
   }

   std::intmax_t number_of_chars_left_in_line()
   {
       if (pos_ >= size_) return 0;
       auto pos = pos_;
       decltype(pos) n = 0;
       for(;ptr_[pos] != '\n' && ptr_[pos] != '\r';++pos,++n) if (pos >= size_) break;
       return n;
   }

   void skip_line()
   {
    if (pos_ >= size_) return;
    auto pos = pos_;
    for(;pos < size_ && ptr_[pos] != '\n' && ptr_[pos] != '\r';++pos);
    for(;pos < size_ && (ptr_[pos] == '\n' || ptr_[pos] =='\r');++pos);
    pos_ = pos;
   }

   ~Memory () {if (ptr_ != nullptr && owner_) delete[] ptr_; }
};

template<typename P, typename C> class Statefulscanner
{
 P p_;
public:

 struct Tokentable
 {
   std::vector<nonmutable_string<C>> toks_;
   std::vector<std::int64_t> toks_descr_tbl_;


   void clear_and_read_table(const char * tbl [], size_t size)
   {
    toks_.resize(size);
    toks_descr_tbl_.resize(size);
    for(size_t i = 0; i < size;++i)
    {
     toks_[i] = nonmutable_string<C>(tbl[i],strlen(tbl[i]));
     toks_descr_tbl_[i] = 0;
    }
   }

   std::int64_t& tok_descr(int tok_id)
   {
    return toks_descr_tbl_[tok_id];
   }

   int match(P& in, char ch)
   {
    if (toks_.size() == 0) return -1;
    bool state_table[1024];
    int states_active = 0;
    int max_state = -1;
    auto nn = toks_.size();
    for(size_t h = 0; h < nn;++h){
      auto & e = toks_[h];
      if (e.length()>0 && e[0] == ch) { state_table[h] = true; max_state = h ;++states_active; }
    }

    if(max_state == -1) return -1;
    int current_pos = 1;
    bool ch_dangling= false;


    do
    {
     if(!(ch_dangling = in.get(ch))) break;

     states_active = 0;
     auto n = max_state;
     for(int i = 0; i <= n;++i)
     {
      if (!state_table[i]) continue;
      auto & e = toks_[i];
      if (e.length() <= current_pos) { state_table[i] = false;continue;}
      if (e[current_pos] == ch) {ch_dangling=false;max_state = i;++states_active;}
      else state_table[i] = false;
     }
     ++current_pos;
    } while (states_active > 0);
    if (ch_dangling) in.unget();
    return max_state;
   }

 };

 Tokentable tokentable_;
 Tokentable& tokentable() {return tokentable_;}

 struct Token
 {
  Token() = default;
  Token(Token const & rhs)=default;

  bool return_ = false;
  int a;
  int kind_ = TOK_IGNORE;
  int b;
  nonmutable_string<C> sval_;
  std::int64_t tok_descr_;
  bool& return_value() {return return_;}
  union
  {
    int ival_;
    std::int16_t ival16_;
    std::int32_t ival32_;
    std::int64_t ival64_;
    float fval_;
    double dval_;
   } v;
   enum {
    TOK_UNKNOWN=0,TOK_ID = 1,TOK_OP = 2,TOK_STR = 3,TOK_UTF8 = 4, TOK_INT16 = 5, TOK_INT32 = 6, TOK_INT64 = 7, TOK_DOUBLE = 8, TOK_FLOAT = 9,
    TOK_TOK = 10, TOK_LEXCMDS = 11, TOK_ENDL = 12, TOK_MATCH_REF = 13, TOK_SIDEEFFECT_STMTS = 14, TOK_SIDEEFFECT_EXPR = 15,TOK_IGNORE = 16,
    TOK_MATCH_ANY = 17, TOK_CALL_LEXER, TOK_ELSE, TOK_EXIT, TOK_REWIND, TOK_ON_ENTER,TOK_ON_EXIT,TOK_BLANK

    };
   int kind() const {return kind_;}
   int & kind()  {return kind_;}
   bool is_numeric() const {return kind_== TOK_INT16 ||  kind_== TOK_INT32 ||  kind_== TOK_INT64 ||  kind_== TOK_FLOAT ||  kind_== TOK_DOUBLE;}

   bool is_zero() const
   {
      if(is_numeric())
      {
       if (kind_== TOK_INT64) return 0 == v.ival64_;
       else if (kind_== TOK_DOUBLE) return 0.0 == v.dval_;
       else if (kind_== TOK_FLOAT) return 0.0 == v.fval_;
       else if (kind_== TOK_INT32) return 0 == v.ival32_;
       else if (kind_== TOK_INT16) return 0 == v.ival16_;
      }
      else return sval_.size() == 0;
   }

   bool bool_value() const { return !is_zero(); }
   std::int64_t get_int() const {
    if (!is_integer()) return 0;
    if (kind_== TOK_INT64) return v.ival64_;
    else if (kind_== TOK_INT32) return v.ival32_;
    else return v.ival16_;
   }

   double get_double() const {
    if (kind_== TOK_FLOAT) return v.fval_;
    else if (kind_== TOK_DOUBLE) return v.dval_;
    return 0.0;
   }

   bool is_integer() const { if (kind_== TOK_INT64) return true;else if (kind_== TOK_INT32) return true;else if (kind_== TOK_INT16) return true; return false;}
   bool is_floatingpoint() const { if (kind_== TOK_FLOAT) return true;else if (kind_== TOK_DOUBLE) return true;return false;}

   bool operator == (Token const & rhs) const
   {
    if (is_integer() && rhs.is_integer())
      return get_int() == rhs.get_int();
    else if (is_floatingpoint())
      return get_double() == rhs.get_double();
    //if (rhs.kind() != kind()) return false;
    return rhs.sval_ == sval_;
   }

   bool operator != (Token const & rhs) const
   {
      return ! operator ==(rhs);
   }
 };

 bool ignore_space_ = false;
 bool skip_unmatched_tokens_ = false;

 bool gettoken_local(Token& t)
 {
    t.kind() = Token::TOK_UNKNOWN;
    char ch;
    auto start = p_.pos_;
    bool starts_with_digit = false;
    for(;p_.get(ch);)
    {
        //std::cerr << ch;
        if (std::isalpha(ch))
        {

          bool not_eof = true;
          for (; (not_eof = p_.get(ch));)
           if (!std::isalnum(ch) && ch != '_') break;
          if (not_eof) p_.unget();
          t.kind() = Token::TOK_ID;
          t.sval_ = nonmutable_string<C>(p_.ptr_+start,p_.pos_-start);
        }
        else if (ch == '"')
        {
          ++start;
          bool not_eof = true;
          for (; (not_eof = p_.get(ch));)
           if (ch == '/' && p_.next_equals('"')) p_.get(ch);
           else if (ch == '"') break;
          if (not_eof) p_.unget();
          t.kind() = Token::TOK_STR;
          t.sval_ = nonmutable_string<C>(p_.ptr_+start,p_.pos_-start);
          if (not_eof) p_.get(ch);
        }
        else if ( /*ch == '.' &&  ||*/ (starts_with_digit = std::isdigit(ch)) )
        {
          bool dot_read = !starts_with_digit;
          bool not_eof = true;
          for (; (not_eof = p_.get(ch));)
          {

           if ( !std::isdigit(ch) && (ch != '.' || ch == '.' && dot_read)  ) break;
           if (ch == '.') dot_read = true;
          }

          if(not_eof && (ch == 'e' || ch == 'E') ){
              dot_read = true;
              p_.get(ch);
              for (; (not_eof = p_.get(ch));)
                  if (!std::isdigit(ch)) break;
          }

          if (not_eof) p_.unget();

          t.sval_ = nonmutable_string<C>(p_.ptr_+start,p_.pos_-start);

          char buffer[128] = {0};

          memcpy(buffer,p_.ptr_+start,p_.pos_-start);


          if (dot_read)
          {
            t.v.dval_ = std::strtod(buffer,nullptr);
            t.kind_ = Token::TOK_DOUBLE;
          }
          else
          {
            t.v.ival64_ = std::strtol(buffer,nullptr,10);
            t.kind_ = Token::TOK_INT64;
          }
        }
        else if (ch == '/' && p_.next_equals('/') )
        {
          p_.skip_line();start = p_.pos_;continue;
        }
        else if (ch == '/' && p_.next_equals('*') )
        {
          p_.get(ch);
          for(;p_.get(ch);) if (ch == '*' && p_.next_equals('/')) { p_.get(ch);break;}
          start = p_.pos_;continue;
        }
        else if (ch == '%' && std::isalpha(p_.next()) )
        {
          start = p_.pos_;
          bool not_eof = true;
          for (; (not_eof = p_.get(ch));)
           if (!std::isalnum(ch) && ch != '_') break;
          if (not_eof) p_.unget();
          auto keyword = nonmutable_string<C>(p_.ptr_+start,p_.pos_-start);
          //std::cout << ">>>>>>>>>>>>'" << keyword << "'\n";
          p_.skip_line();
          start = p_.pos_;
          if (keyword == "ignore_ws") ignore_space_ = true;
          else if (keyword == "skip_unmatched_tokens") skip_unmatched_tokens_ = true;
          continue;
        }
        else if (ch == '%'  && p_.next_equals('%') && p_.number_of_chars_left_in_line()==1)
        {
          p_.skip_line();
          t.kind() =  Token::TOK_LEXCMDS;
          return true;
        }
        else if (ignore_space_ && std::isspace(ch))
        {
          start = p_.pos_;continue;
        }
        else if (!ignore_space_ && std::isspace(ch))
        {
          t.kind() = Token::TOK_BLANK;t.sval_ = nonmutable_string<C>(p_.ptr_+start,0);
        }
        else if (ch == '\n') {t.kind() = Token::TOK_ENDL;t.sval_ = nonmutable_string<C>(p_.ptr_+start,0);}
        else
        {
         //operators

         auto tt = p_.pos();
         int op_id = tokentable().match(p_,ch);

         if (op_id == -1) { t.kind() = Token::TOK_UNKNOWN;p_.pos()=tt;return true;}
         t.kind() = Token::TOK_TOK;
         if (ch != '\n') t.sval_ = nonmutable_string<C>(p_.ptr_+start,p_.pos_-start);
         else t.sval_ = nonmutable_string<C>(p_.ptr_+start,0);
         t.v.ival32_ = op_id;
        }
        return true;
    }
    return false;
 }

 // Rewrite-logic
 struct rw_opcode
 {
    bool match_type_;
    bool match_content_;
    std::int16_t ref_;
    Token t_;
    rw_opcode():match_type_(false),match_content_(false),ref_(-1),t_(Token()) {t_.kind() = Token::TOK_UNKNOWN;}
    rw_opcode(rw_opcode const & rhs)
     :match_type_(rhs.match_type_), match_content_(rhs.match_content_),ref_(rhs.ref_), t_(rhs.t_)
     {}
    rw_opcode(bool match_type, bool match_content, std::int16_t ref, Token t)
     :match_type_(match_type), match_content_(match_content),ref_(ref), t_(t)
     {}
 };

 struct rw_rule
 {
   bool active_ = false;
   int lexer_id_ = 0;
   int pattern_ = -1;
   int pattern_len_  = 0;
   int body_len_  = 0;
   int layer_ = 0;
   typename P::state_t rewind_state_;
   rw_rule() = default;
   rw_rule(rw_rule const &) = default;
   rw_rule(int lexer_id,bool active, int pattern, int pattern_len, int body_len):
    lexer_id_(lexer_id),active_(active), pattern_(pattern), pattern_len_(pattern_len), body_len_(body_len)
    {}
   int lexer_id() const {return lexer_id_;}
   int& lexer_id()  {return lexer_id_;}
   typename P::state_t& rewind_state(){return rewind_state_;}
 };

 enum {DEFAULT_RW_PROG_BUFFER = 8192};
 enum {MAX_TOKEN_LOOKAHEAD = 1024};
 std::vector<rw_opcode> prog_buffer_;
 std::vector<rw_rule> rw_rules_;
 std::vector<bool> matching_rules_;
 std::map<nonmutable_string<C>,int> lexer_name_to_id_;
 std::map<nonmutable_string<C>,int>& lexer_name_to_id() {return lexer_name_to_id_;}
 int next_free_lexer_id_ = 1;

 int get_lexer_id(nonmutable_string<C> const & lexer_id)
 {
  auto iter = lexer_name_to_id().find(lexer_id);
  if (iter != lexer_name_to_id().end() ) return iter->second;
  lexer_name_to_id()[lexer_id] = next_free_lexer_id_++;
  return next_free_lexer_id_ - 1;
 }


 std::vector<std::vector<Token>> matched_tokens_;
 int current_layer_ = 0;


 int rw_prog_buffer_free_;
/* int rw_rule_in_execution_;
 int rw_rule_in_execution_ip_;*/

 int current_layer() const {return current_layer_;}
 int& current_layer() {return current_layer_;}
 std::vector<Token>& matched_tokens(){
  if (current_layer()>= matched_tokens_.size())
   for(int i =  matched_tokens_.size()-1 ; i < current_layer();++i)
    matched_tokens_.push_back(std::vector<Token>());
  return matched_tokens_[current_layer()];
 }

 Statefulscanner()
 {
   prog_buffer_.resize(DEFAULT_RW_PROG_BUFFER);
   rw_prog_buffer_free_ = 0;
   //rw_rule_in_execution_ = -1;
   matching_rules_.resize(DEFAULT_RW_PROG_BUFFER);
 }

 Statefulscanner(P && p): p_(std::move(p))
 {
   prog_buffer_.resize(DEFAULT_RW_PROG_BUFFER);
   rw_prog_buffer_free_ = 0;
   //rw_rule_in_execution_ = -1;
   matching_rules_.resize(DEFAULT_RW_PROG_BUFFER);
 }

 void set_input(P & p) {p_ = p;}

 struct match_ex{ std::string what_;};
 struct eval_ex{};

 Token expr_parser_lookahead_;

 Token& expr_parser_cur_tok()
 {
  return expr_parser_lookahead_;
 }


 bool expr_parser_match(int kind, const char* content = nullptr, bool check = true, bool b_call_gettoken = true, bool b_throw = true)
 {
  if (check)
   if ( kind != expr_parser_lookahead_.kind() ||  content != nullptr && expr_parser_lookahead_.sval_ != content )
    {if (b_throw) if (content) throw match_ex{std::string("Expected ")+content}; else throw match_ex{std::to_string(kind)}; return false;}

  if (b_call_gettoken)
   if (!gettoken_local(expr_parser_lookahead_)) {
     if (b_throw) if (content) throw match_ex{std::string("Expected ")+content}; else throw match_ex{std::to_string(kind)}; return false;
     }

  return true;
 }

 bool expr_parser_gettoken()
 {
  return expr_parser_match(Token::TOK_IGNORE,nullptr,false,true);
 }


 std::map<nonmutable_string<C>,Token> symbols;
 struct Sideeffects_node{
  enum {STMT,ASSIGN,LEAF,DEBUG_PRINT_SYMBOLS,OR,AND,NOT,EQ,NEQ,GT,LT,CLEAR_SYMBOLS,IF,ELSE,RETURN,MATCH_REF};
  int kind_;
  int left_,right_;
  Token t_;
  Sideeffects_node():left_(-1),right_(-1){}
 };

 std::vector<Sideeffects_node> sideeffects_storage_;

 int get_sideeffects_storage_free_idx() const {return sideeffects_storage_.size();}
 int push_sideeffects_op(Sideeffects_node& s) {int i = sideeffects_storage_.size();sideeffects_storage_.push_back(s);return i;}
 Sideeffects_node& sideeffects_storage(int i){return sideeffects_storage_[i];}


 bool is_true(Token const & t)
 {
  if (t.kind() == Token::TOK_INT64)
   return t.v.ival64_ != 0;
  return false;
 }

 Token evaluate(Sideeffects_node& node)
 {
  if (node.kind_ == Sideeffects_node::RETURN)
  {
    auto r = evaluate(sideeffects_storage(node.left_));
    r.return_value() = true;
    return r;
  }else if (node.kind_ == Sideeffects_node::MATCH_REF)
  {
   return matched_tokens()[node.left_];
  }
  else if (node.kind_ == Sideeffects_node::ASSIGN)
  {
    auto& v = symbols[sideeffects_storage(node.left_).t_.sval_];
    Token tt = evaluate(sideeffects_storage(node.right_));

    return v = tt;
  } else if (node.kind_ == Sideeffects_node::EQ)  {
    auto lhs = evaluate(sideeffects_storage(node.left_));

    auto rhs = evaluate(sideeffects_storage(node.right_));

    bool r = lhs == rhs;
    Token rt;
    if (r) {rt.kind_ = Token::TOK_INT64;rt.v.ival64_ = 1;}
    else {rt.kind_ = Token::TOK_INT64;rt.v.ival64_ = 0;}
    return rt;
  } else if (node.kind_ == Sideeffects_node::NEQ)  {
    auto lhs = evaluate(sideeffects_storage(node.left_));

    auto rhs = evaluate(sideeffects_storage(node.right_));

    bool r = lhs != rhs;
    Token rt;
    if (r) {rt.kind_ = Token::TOK_INT64;rt.v.ival64_ = 1;}
    else {rt.kind_ = Token::TOK_INT64;rt.v.ival64_ = 0;}
    return rt;
  }
  else if (node.kind_ == Sideeffects_node::OR)
  {
    auto lhs = evaluate(sideeffects_storage(node.left_));
    bool r = lhs.bool_value(); if (r) return lhs;
    auto rhs = evaluate(sideeffects_storage(node.right_));
    return rhs;
  } else if (node.kind_ == Sideeffects_node::AND)  {
    auto lhs = evaluate(sideeffects_storage(node.left_));
    bool r = lhs.bool_value(); if (!r) return lhs;
    auto rhs = evaluate(sideeffects_storage(node.right_));
    return rhs;
  } else if (node.kind_ == Sideeffects_node::NOT)  {
    auto r = evaluate(sideeffects_storage(node.left_));
    if (r.kind() == Token::TOK_INT64)
    {
     if (r.v.ival64_ == 0) r.v.ival64_ = 1; else r.v.ival64_ = 0;
    }
    return r;
  }
  else if (node.kind_ == Sideeffects_node::CLEAR_SYMBOLS)
  {
    symbols.clear();
    node.t_.kind() = Token::TOK_IGNORE;
    return node.t_;
  }
  else if (node.kind_ == Sideeffects_node::DEBUG_PRINT_SYMBOLS)
  {
    std::cerr << "\n************SYMBOL_TABLE************\n";
    for(auto const & e : symbols)
    {
     std::cerr << "\t" << e.first << " = ";
     if (e.second.kind() == Token::TOK_INT64)
      std::cerr << e.second.v.ival64_;
     std::cerr << "\n";
    }
    std::cerr << "************************************\n";
    node.t_.kind() = Token::TOK_IGNORE;
    return node.t_;
  }
  else if (node.kind_ == Sideeffects_node::LEAF)
  {

   if (node.t_.kind() == Token::TOK_ID)
   {
    auto sym_iter = symbols.find(node.t_.sval_);
    if (sym_iter == symbols.end())
    {

      symbols[node.t_.sval_].kind() = Token::TOK_INT64;
      symbols[node.t_.sval_].v.ival64_ = 0;
    }
    return symbols[node.t_.sval_];
   }
   else return node.t_;

  }
  else if (node.kind_ == Sideeffects_node::IF)
  {
   Token c = evaluate (sideeffects_storage(node.left_));
   bool b = is_true(c);

   if (b && sideeffects_storage(node.right_).kind_ == Sideeffects_node::STMT)
    {return evaluate (sideeffects_storage(node.right_));}
   else if (!b && sideeffects_storage(node.right_).kind_ == Sideeffects_node::STMT)
    return {};
   else if (b && sideeffects_storage(node.right_).kind_ == Sideeffects_node::ELSE)
    {return evaluate(sideeffects_storage(sideeffects_storage(node.right_).left_));}

   return evaluate(sideeffects_storage(sideeffects_storage(node.right_).right_));
  }


  std::cerr << "\nUnknown opcode: " << node.kind_ << std::endl;
  throw eval_ex();

 }

 bool evaluate_guard(rw_rule& rule)
 {
  if (rule.body_len_ == 0) return true;
  for(int i = 0; i < rule.body_len_;++i)
  {
    auto& opcode = prog_buffer_[rule.pattern_+rule.pattern_len_+i];
    if (opcode.t_.kind() == Token::TOK_SIDEEFFECT_EXPR)
    {
      if (opcode.t_.v.ival64_>=0)
      {
        Token result = evaluate(sideeffects_storage(opcode.t_.v.ival64_));
        if (result.kind() == Token::TOK_INT64 && result.v.ival64_ == 0) return false;
      }
    }
  }
  return true;
 }

Token evaluate_sideeffect(std::int64_t root)
 {
  Token t;
  for(;;)
  {
    if (root == -1) break;
    auto& node = sideeffects_storage(root);
    if(node.kind_ != Sideeffects_node::STMT) break;
    if(node.left_ != -1)
     t = evaluate ( sideeffects_storage(node.left_) );
    root = node.right_;
  }
  return t;
 }

 int parse_sideeffect_factor()
 {
  Token t = expr_parser_cur_tok();
  Sideeffects_node s;
  s.kind_ = Sideeffects_node::LEAF;
  s.t_ = t;

  if (t.kind() == Token::TOK_ID && t.sval_ == "print_symbols")
  {
    expr_parser_gettoken();
    Sideeffects_node s;
    s.kind_ = Sideeffects_node::DEBUG_PRINT_SYMBOLS;
    s.left_ = -1;
    s.right_ = -1;
    return push_sideeffects_op(s);
  }
  else if (t.kind() == Token::TOK_ID && t.sval_ == "return")
  {
   expr_parser_gettoken();
   auto i = parse_sideeffect_expr();
   Sideeffects_node s;
   s.kind_ = Sideeffects_node::RETURN;
   s.left_ = i;
   s.right_ = -1;
   return push_sideeffects_op(s);
  } else if (t.kind() == Token::TOK_TOK && t.sval_ == "$")  {
   expr_parser_gettoken();
   if (expr_parser_cur_tok().kind() != Token::TOK_INT64)
    throw match_ex(/*"Expected positive integer after $."*/);

   Sideeffects_node s;
   s.kind_ = Sideeffects_node::MATCH_REF;
   s.left_ = expr_parser_cur_tok().v.ival64_;
   s.right_ = -1;
   expr_parser_gettoken();
   return push_sideeffects_op(s);
  }
  else if (t.kind() == Token::TOK_ID || t.kind() == Token::TOK_STR || t.kind() == Token::TOK_INT64)
  {
    expr_parser_gettoken();
    return push_sideeffects_op(s);
  }
  else if (t.kind() == Token::TOK_TOK && t.sval_ == "(")
  {
   expr_parser_gettoken();
   auto i = parse_sideeffect_expr();
   expr_parser_match(Token::TOK_TOK,")");
   return i;
  }
  else if (t.kind() == Token::TOK_TOK && t.sval_ == "!")
  {
   expr_parser_match(Token::TOK_TOK,"!");
   auto i = parse_sideeffect_factor();

   Sideeffects_node s;
   s.kind_ = Sideeffects_node::NOT;
   s.left_ = i;
   s.right_ = -1;

   return push_sideeffects_op(s);
  }
  else throw match_ex();
 }

int parse_sideeffect_eq_neq()
 {
  auto r = parse_sideeffect_factor();

  while ( (expr_parser_cur_tok().kind() == Token::TOK_TOK) && ( (expr_parser_cur_tok().sval_ == "==") || (expr_parser_cur_tok().sval_ == "!=") ) )
  {
    bool is_eq = expr_parser_cur_tok().sval_ == "==";

    if (is_eq) expr_parser_match(Token::TOK_TOK,"==");
    else expr_parser_match(Token::TOK_TOK,"!=");

    auto rhs = parse_sideeffect_factor();
    Sideeffects_node s;
    s.kind_ = is_eq ? Sideeffects_node::EQ : Sideeffects_node::NEQ;
    s.left_ = r;
    s.right_ = rhs;
    r = push_sideeffects_op(s);
  }//while

 return r;
}

int parse_sideeffect_and()
 {
  auto r = parse_sideeffect_eq_neq();
  while (expr_parser_cur_tok().kind() == Token::TOK_TOK && expr_parser_cur_tok().sval_ == "&&" )
  {
    expr_parser_match(Token::TOK_TOK,"&&");
    auto rhs = parse_sideeffect_eq_neq();
    Sideeffects_node s;
    s.kind_ = Sideeffects_node::AND;
    s.left_ = r;
    s.right_ = rhs;
    r = push_sideeffects_op(s);
  }//while
 return r;
}

int parse_sideeffect_or()
 {
  auto r = parse_sideeffect_and();
  while (expr_parser_cur_tok().kind() == Token::TOK_TOK && expr_parser_cur_tok().sval_ == "||" )
  {
    expr_parser_match(Token::TOK_TOK,"||");
    auto rhs = parse_sideeffect_and();
    Sideeffects_node s;
    s.kind_ = Sideeffects_node::OR;
    s.left_ = r;
    s.right_ = rhs;
    r = push_sideeffects_op(s);
  }//while
 return r;
}

int parse_sideeffect_ASSIGN()
 {
 auto r = parse_sideeffect_or();

 if (expr_parser_cur_tok().kind() == Token::TOK_TOK && (expr_parser_cur_tok().sval_ == "=" ) )
 {
  expr_parser_match(Token::TOK_TOK,"=");
  auto rhs = parse_sideeffect_or();
  Sideeffects_node s;
  s.kind_ = Sideeffects_node::ASSIGN;
  s.left_ = r;
  s.right_ = rhs;
  r = push_sideeffects_op(s);
  }
 return r;
}

 int parse_sideeffect_expr()
 {
   return  parse_sideeffect_ASSIGN();
 }

 int parse_sideeffect_stmt()
 {

  if (expr_parser_cur_tok().kind() == Token::TOK_ID && expr_parser_cur_tok().sval_ == "if")
  {
   expr_parser_match(Token::TOK_ID,"if");
   expr_parser_match(Token::TOK_TOK,"(");
   auto cond = parse_sideeffect_expr();
   expr_parser_match(Token::TOK_TOK,")");


   auto if_branch =  parse_sideeffect_stmt();
   int else_branch = -1;

   if (expr_parser_cur_tok().kind() == Token::TOK_ID && expr_parser_cur_tok().sval_ == "else" )
   {
    expr_parser_match(Token::TOK_ID,"else");
    else_branch = parse_sideeffect_stmt();
   }

   Sideeffects_node s;
   s.kind_=  Sideeffects_node::IF;
   s.left_= cond;
   if(else_branch == -1)
    s.right_ = if_branch;
   else
   {
    Sideeffects_node t;
    t.kind_=  Sideeffects_node::ELSE;
    t.left_ = if_branch;
    t.right_ = else_branch;
    s.right_ = push_sideeffects_op(t);
   }
   return push_sideeffects_op(s);
  }
  auto r = parse_sideeffect_expr();

  expr_parser_match(Token::TOK_TOK,";");

  return r; 
 }

 int parse_sideeffect()
 {
   //std::cout << "parse_sideeffect()\n";
   expr_parser_gettoken();
   Sideeffects_node s;
   s.kind_ = Sideeffects_node::STMT;
   s.left_ = s.right_ = -1;

   int root = push_sideeffects_op(s);
   int cur_node = root;

   for(;;)
   {
    int idx = parse_sideeffect_stmt();

    if (idx != -1)
    {

     if (sideeffects_storage(cur_node).left_ == -1)
     {

      sideeffects_storage(cur_node).left_ = idx;

     }
     else
     {
      int old_node_idx = cur_node;

      Sideeffects_node s;
      s.kind_ = Sideeffects_node::STMT;
      s.left_ = idx;
      s.right_ = -1;
      cur_node = push_sideeffects_op(s);
      sideeffects_storage(old_node_idx).right_ = cur_node;


     }
    }
    if (expr_parser_cur_tok().kind() == Token::TOK_TOK && expr_parser_cur_tok().sval_ == "/" ) break;

   }//for
   return root;
 }

 void read_rewrite_rule(int lexer_id)
 {

  //std::cout << "****read_rewrite_rule()\n";
  int guard_condition = -1;

  //pattern part
  auto pat_start = rw_prog_buffer_free_;
  auto pat_len = 0;

  for(;;expr_parser_gettoken())
  {
   Token t = expr_parser_cur_tok();


   if (t.kind() == Token::TOK_TOK && t.sval_ == "=>") break;

   if (t.kind() == Token::TOK_TOK && t.sval_ == "\\")
   {
      expr_parser_gettoken();
      t = expr_parser_cur_tok();
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++pat_len;
   }
   else if (t.kind() == Token::TOK_INT64) {
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++pat_len;
    }
   else if (t.kind() == Token::TOK_ID && t.sval_ == "endl") {
      t.kind() = Token::TOK_ENDL;
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++pat_len;
    }
   else if (t.kind() == Token::TOK_ID && t.sval_ == "enter") {
	  t.kind() = Token::TOK_ON_ENTER;
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++pat_len;
   }
   else if (t.kind() == Token::TOK_ID && t.sval_ == "exit") {
      t.kind() = Token::TOK_ON_EXIT;
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++pat_len;
   }
   else if (t.kind() == Token::TOK_ID && t.sval_ == "string")
   {
      t.kind_ = Token::TOK_STR;
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,false,-1,t);
      ++pat_len;
   }
   else if (t.kind() == Token::TOK_ID && (t.sval_ == "integer" || t.sval_ == "int" ) )
   {
      t.kind_ = Token::TOK_INT64;
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,false,-1,t);
      ++pat_len;
   } else if (t.kind() == Token::TOK_ID && t.sval_ == "double"  )
   {
      t.kind_ = Token::TOK_DOUBLE;
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,false,-1,t);
      ++pat_len;
   }

    else if (t.kind() == Token::TOK_ID && t.sval_ == "ident")
   {
      t.kind_ = Token::TOK_ID;
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,false,-1,t);
      ++pat_len;
   }
   else if (t.kind() == Token::TOK_ID && t.sval_ == "any")
   {
      t.kind_ = Token::TOK_MATCH_ANY;
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(false,false,-1,t);
      ++pat_len;
   }
   else if (t.kind() == Token::TOK_ID && t.sval_ == "else")
   {
      t.kind_ = Token::TOK_ELSE;
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(false,false,-1,t);
      ++pat_len;
   }

   else if (t.kind() == Token::TOK_STR) {
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++pat_len;
    }
   else if (t.kind() == Token::TOK_ID)
   {

      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++pat_len;
   }
   else if (t.kind() == Token::TOK_TOK && t.sval_ == "|")
   {   
     expr_parser_gettoken();
     guard_condition = parse_sideeffect_expr();
     break;
   }
   else if (t.kind() == Token::TOK_TOK)
   {
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++pat_len;
   }
   else {

     throw match_ex();
     }
  }//for


  expr_parser_match(Token::TOK_TOK,"=>");



  auto body_start = pat_start;
  auto body_len = 0;

  if (guard_condition >= 0)
  {
    Token t;
    t.kind() = Token::TOK_SIDEEFFECT_EXPR;
    t.v.ival64_ = guard_condition;
    //std::cout << "guard="<< guard_condition << "\n";
    prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
    //std::cout << "guard="<< prog_buffer_[rw_prog_buffer_free_-1].t_.v.ival64_<< "\n";
    ++body_len;
  }

  for(;;expr_parser_gettoken())
  {

   Token t = expr_parser_cur_tok();

   if (t.kind() == Token::TOK_INT64)
    {
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++body_len;
    }
   else if (t.kind() == Token::TOK_TOK && t.sval_ == ".") break;
   else if (t.kind() == Token::TOK_TOK && t.sval_ == "/")
   {
      int script_ast = parse_sideeffect();

      if (! (t.kind() == Token::TOK_TOK && t.sval_ == "/") ) throw match_ex();
      t.kind() = Token::TOK_SIDEEFFECT_STMTS;
      t.v.ival64_ = script_ast;
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++body_len;
   }
   else if (t.kind() == Token::TOK_ID && t.sval_ == "null")
   {
      t.kind() = Token::TOK_IGNORE;
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++body_len;
   }
   else if (t.kind() == Token::TOK_ID && t.sval_ == "endl") {
      t.kind() = Token::TOK_ENDL;
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++body_len;
    }else if (t.kind() == Token::TOK_ID && t.sval_ == "blank") {
       t.kind() = Token::TOK_BLANK;
       prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
       ++body_len;
     }
   else if (t.kind() == Token::TOK_TOK && t.sval_ == "$")
   {
    if (!gettoken_local(t)) throw match_ex();
    if (t.kind() == Token::TOK_INT64)
    {
      t.kind() = Token::TOK_MATCH_REF;
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++body_len;
    }else throw match_ex();

   }else if (t.kind() == Token::TOK_ID && t.sval_ == "exit") {
      t.kind_ = Token::TOK_EXIT;
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++body_len;
   }
   else if (t.kind() == Token::TOK_ID && t.sval_ == "rewind") {
      t.kind_ = Token::TOK_REWIND;
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++body_len;
   }
   else if (t.kind() == Token::TOK_ENDL) {
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++body_len;
   } else if (t.kind() == Token::TOK_STR) {

      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++body_len;
   } else if (t.kind() == Token::TOK_ID && t.sval_ == "call")
   {
      expr_parser_gettoken();
      if (Token::TOK_ID != expr_parser_cur_tok().kind()) throw match_ex{"identifier expected"};
      auto l_id = this->lexer_name_to_id()[expr_parser_cur_tok().sval_];
      t.kind_ = Token::TOK_CALL_LEXER;
      t.v.ival64_ = l_id;
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++body_len;
   }
   else if (t.kind() == Token::TOK_ID)
   {
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++body_len;
   }
   else if (t.kind() == Token::TOK_TOK)
   {
      prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
      ++body_len;
   }
   else throw match_ex{"read_rewrite_rule(): rhs faulty."};
  }
  rw_rules_.push_back(rw_rule(lexer_id,true,pat_start,pat_len,body_len));
 }

 void read_lex_cmds(int lexer_id)
 {
  //std::cout << "****read_lex_cmds()\n";

  //std::cout << "'" << expr_parser_cur_tok().sval_ << "'\n";
  //std::cout << (expr_parser_cur_tok().kind() ==  Token::TOK_TOK) << "\n";
  expr_parser_match(Token::TOK_TOK,"{",true,false);
  //std::cout << expr_parser_cur_tok().sval_ << "\n";
  for(;;)
  {
    expr_parser_gettoken();
    if (expr_parser_cur_tok().kind() == Token::TOK_TOK && expr_parser_cur_tok().sval_ == "}") return;
    else read_rewrite_rule(lexer_id);
  }
 }



 void process_lexcommands()
 {
    //std::cout << "****process_lexcommands()\n";
    Token t;
    while(expr_parser_gettoken())
    {
      t = expr_parser_cur_tok();
      if (t.kind() == Token::TOK_LEXCMDS)  break;

      try {
        if (t.kind() == Token::TOK_ID && t.sval_ == "BEGIN")
        {
          expr_parser_gettoken();read_lex_cmds(0);
        } else if (t.kind() == Token::TOK_ID && t.sval_ == "lexer") {
         expr_parser_gettoken();
         auto lexer_id_s = expr_parser_cur_tok().sval_;
         auto lexer_id = get_lexer_id(lexer_id_s);

         expr_parser_gettoken();
         read_lex_cmds(lexer_id);
        }
       } catch (match_ex& ex)
        {std::cerr << "***Lexer script faulty:" << ex.what_ << "\n";throw ex;while(gettoken_local(t))if (t.kind() == Token::TOK_LEXCMDS)  break; return; }

    }
 }

 bool active_rewrites(int lexer_id)
  { return rw_rules_.size() > 0; }


 bool rw_rule_fetch_and_dispatch(Token& t, int rwr, int& rwr_ip)
 {
  auto n = rw_rules_[rwr].body_len_;
  auto start = rw_rules_[rwr].pattern_ + rw_rules_[rwr].pattern_len_;
  if (rwr_ip-start >= n ) return false;

  auto const& opcode = prog_buffer_[rwr_ip++];
  t = opcode.t_;
  if (t.kind() == Token::TOK_MATCH_REF)
  {

   t = matched_tokens()[t.v.ival64_];

  }

  return true;
 }


 struct lex_env{
  int rw_rule_in_execution_ = -1;
  int rw_rule_in_execution_ip_ = -1;
  //bool token_pending_ = false;
  bool exit_called_ = false;
  bool on_enter_executed = false;
  bool on_exit_executed = false;

  //Token t_;

  lex_env() = default;
  //Token& pending_token() {token_pending_=true;return t_;}
 };

 std::vector<lex_env> lex_envs_;
 size_t max_env_stacking_ = 1024;
 int cur_env_ = 0;

 bool gettoken(Token& t)
 {
  if (lex_envs_.size() == 0)
   lex_envs_.resize(max_env_stacking_);
  cur_env_ = 0;
  return gettoken(t,0);
 }

 bool activate_toplevel_onexit_trigger_ = true;
 void activate_toplevel_onexit_trigger(bool b) {activate_toplevel_onexit_trigger_ = b;}

 bool trigger_on_exit(int lexer_id)
 {

	 if (cur_env_ == 0 && !activate_toplevel_onexit_trigger_) return false;

	 auto& env = lex_envs_[cur_env_];
	 if (!env.on_exit_executed && rw_rules_.size() > 0)
	 {
		 env.on_exit_executed = true;
		 auto n = rw_rules_.size();
		 int exit_rule = -1;
		 for (int i = 0; i < n;++i) {
		  if (lexer_id != rw_rules_[i].lexer_id_)continue;
		  if (rw_rules_[i].pattern_len_ == 1 &&  prog_buffer_[rw_rules_[i].pattern_].t_.kind() == Token::TOK_ON_EXIT) {exit_rule=i;break;}
		 }
	 	 if (exit_rule >= 0 && rw_rules_[exit_rule].body_len_ > 0) {
	 		 env.rw_rule_in_execution_ = exit_rule;
		 	 env.rw_rule_in_execution_ip_ = rw_rules_[env.rw_rule_in_execution_].pattern_ + rw_rules_[env.rw_rule_in_execution_].pattern_len_;
		 }
	 	 if (exit_rule >= 0) { return true;}
	 }
	 return false;
 }

 bool gettoken(Token& t,int lexer_id)
 {

  auto& env = lex_envs_[current_layer_ = cur_env_];
  for(;;)
  {
   if (!env.on_enter_executed && rw_rules_.size() > 0)
   {
	 auto n = rw_rules_.size();

	 int enter_rule = -1;
	 for (int i = 0; i < n;++i) {
	  if (lexer_id != rw_rules_[i].lexer_id_)continue;

	  if (rw_rules_[i].pattern_len_ == 1 &&  prog_buffer_[rw_rules_[i].pattern_].t_.kind() == Token::TOK_ON_ENTER) {enter_rule=i;break;}
	 }

	 if (enter_rule >= 0 && rw_rules_[enter_rule].body_len_ > 0) {

		 env.rw_rule_in_execution_ = enter_rule;
		 env.rw_rule_in_execution_ip_ = rw_rules_[env.rw_rule_in_execution_].pattern_ + rw_rules_[env.rw_rule_in_execution_].pattern_len_;
	 }
	 env.on_enter_executed = true;
   }
   /*check whether we have to  write the rhs of a previously matched  pattern.*/
   if (env.rw_rule_in_execution_ != -1)
   {
    int old_rw_rule_in_execution_ip = env.rw_rule_in_execution_ip_;
    if (rw_rule_fetch_and_dispatch(t,env.rw_rule_in_execution_, env.rw_rule_in_execution_ip_))
    {
     if (t.kind() == Token::TOK_SIDEEFFECT_STMTS)
     {
      if (t.v.ival64_ >= 0) {
       auto r = evaluate_sideeffect(t.v.ival64_);
       if (r.kind_ != Token::TOK_IGNORE && r.return_value()) {t = r;return true;}
       continue;
      }
      continue;
     } else if (t.kind() == Token::TOK_SIDEEFFECT_EXPR) continue;
     else if (t.kind() == Token::TOK_CALL_LEXER)
     {
      auto l_id = t.v.ival64_;

      auto tt = cur_env_++;
      bool r = gettoken(t,l_id);
      cur_env_ = tt;

      if (r) { env.rw_rule_in_execution_ip_ = old_rw_rule_in_execution_ip; return true;}
      else {        
        lex_envs_[cur_env_+1] = lex_env();
      }
      continue;
     }
     else if (t.kind() == Token::TOK_IGNORE) continue;
     else if (t.kind() == Token::TOK_EXIT) {

        return false;
     }
     else if(t.kind() == Token::TOK_REWIND)
     {
      p_.read_state( rw_rules_[env.rw_rule_in_execution_].rewind_state());
      continue;
     }
     return true;
    }
    env.rw_rule_in_execution_ = -1;
   }

   auto state_raw_lexer_before_gettoken_local = this->p_.get_state();

   bool r = gettoken_local(t);
   Token orig_t = t;


   if (!r) {if (trigger_on_exit(lexer_id)) continue; return r;}

   if (t.kind() == Token::TOK_LEXCMDS)
       {
         bool ignore_ws_temp = ignore_space_;
         ignore_space_ = true;
         process_lexcommands();
         ignore_space_=ignore_ws_temp;
         continue;
       }
   if (!active_rewrites(lexer_id))
    return r;

   //INVARIANT: There are active rewrite rules
   auto n = rw_rules_.size();

   bool active_state_with_composite_pattern = false;
   int current_pos = 0;
   std::vector<bool> guard_is_true(n);
   for (int i = 0; i < n;++i) {
    matching_rules_[i] = false;
    if (lexer_id != rw_rules_[i].lexer_id_){guard_is_true[i] = false;continue;}
    guard_is_true[i] = evaluate_guard(rw_rules_[i]);
   }

   int min_matching_idx = INT_MAX;
   int last_min_matching_idx = INT_MAX;
   auto state_raw_lexer = this->p_.get_state();

   bool dangling_token = false;

   matched_tokens().clear();matched_tokens().push_back(t);

   int else_state = -1;
   //if(orig_t.sval_=="BO_") std::cout << ">>START MATCH\n";
   do
   {
    last_min_matching_idx = min_matching_idx;
    //min_matching_idx = INT_MAX;
    active_state_with_composite_pattern = false;
    for(decltype(n) i = 0; i < n;++i)
    {
     auto & cur_rule = rw_rules_[i];
     //std::cout << cur_rule.pattern_ <<" mt:" << prog_buffer_[cur_rule.pattern_].match_type_ << " mct:"<< prog_buffer_[cur_rule.pattern_].match_content_<< std::endl;
     //std::cout << "rw_rules_[i].pattern_len_=" << rw_rules_[i].pattern_len_ << "\n";
     //std::cout << "prog_buffer_[rw_rules_[i].pattern_].t_.kind() =" << prog_buffer_[rw_rules_[i].pattern_].t_.kind() << std::endl;

     if (!guard_is_true[i]) continue;
     bool previous_it_matched = matching_rules_[i] || current_pos == 0;

     if (cur_rule.pattern_len_ > 0 && cur_rule.pattern_len_ <= current_pos)
        continue;

     if (!previous_it_matched) continue;

     if (cur_rule.pattern_len_ == 0){ //pattern is empty ==> triggers always
      if (min_matching_idx > i)min_matching_idx = i;
      matching_rules_[i] = true;
      continue;
     }

     auto & opcode = prog_buffer_[cur_rule.pattern_+current_pos];

     if (!opcode.match_type_ && !opcode.match_content_ && opcode.t_.kind() == Token::TOK_MATCH_ANY)
     {
       if (min_matching_idx > i && (cur_rule.pattern_len_ - current_pos == 1) )
       {
        min_matching_idx = i;
        state_raw_lexer = this->p_.get_state();
       }
       if (cur_rule.pattern_len_ - current_pos > 1) active_state_with_composite_pattern = true;
       matching_rules_[i] = true;
     } else if (opcode.t_.kind() == Token::TOK_ELSE)
     {
       else_state = i;
     } else if ( opcode.match_type_ && !opcode.match_content_ && t.kind() == opcode.t_.kind()){
          /**only type match required**/
       //std::cout <<  "opcode.match_type_ && !opcode.match_content_ && t.kind() == opcode.t_.kind()\n";
       if (min_matching_idx > i && (cur_rule.pattern_len_ - current_pos == 1) )
       {
        min_matching_idx = i;
        state_raw_lexer = this->p_.get_state();
       }
       if (cur_rule.pattern_len_ - current_pos > 1) active_state_with_composite_pattern = true;
       matching_rules_[i] = true;

     } else if ( opcode.match_type_ && opcode.match_content_ && t.kind() == opcode.t_.kind()) {
       /**matching type of token and content**/
       bool match = false;
       if (t.kind() == Token::TOK_INT64){
         if (t.v.ival64_ == opcode.t_.v.ival64_)match = true;
       } else if (t.kind() == Token::TOK_ENDL) match = true;
       else if (t.kind() == Token::TOK_ID && t.sval_ == opcode.t_.sval_)
       {
    	   match = true;
       }
       else if (t.kind() == Token::TOK_STR && t.sval_ == opcode.t_.sval_)match = true;
       else if (t.kind() == Token::TOK_TOK && t.sval_ == opcode.t_.sval_)match = true;

       if (!match) matching_rules_[i] = false;
       else{
        if (min_matching_idx > i && (cur_rule.pattern_len_ - current_pos == 1) ){
          min_matching_idx = i; state_raw_lexer = p_.get_state();
        }
        if (cur_rule.pattern_len_ - current_pos > 1) active_state_with_composite_pattern = true;
        matching_rules_[i] = true;
       }
     } else matching_rules_[i] = false;

    }//for
    ++current_pos;
    if (active_state_with_composite_pattern){
     dangling_token = gettoken_local(t);
     if (!dangling_token) {active_state_with_composite_pattern = false;}
     else matched_tokens().push_back(t);

    }
    //if(orig_t.sval_=="BO_") std::cout << min_matching_idx << "\n";
    } while(active_state_with_composite_pattern);

   //if(orig_t.sval_=="BO_") std::cout << ">>END MATCH\n";
   //if(orig_t.sval_=="BO_") std::cout << "******Selected rule: #" << min_matching_idx  <<" "<< orig_t.kind() <<"\n";
   p_.read_state(state_raw_lexer);
   if (min_matching_idx >= n && last_min_matching_idx < n) min_matching_idx = last_min_matching_idx;
   if (min_matching_idx >= n && else_state == -1) {
    if (skip_unmatched_tokens_)
    {
    	if(!r){t = orig_t; return r;}
    	else continue;
    }
    else {t = orig_t; return r;}
   } else if (min_matching_idx >= n && else_state >= 0)
   {
    min_matching_idx = else_state;
    p_.read_state(state_raw_lexer_before_gettoken_local);
   }

   //if (orig_t.sval_ == "VAL_TABLE_") std::cout << "******Selected rule: #" << min_matching_idx  << "\n";
   //std::cout << "******Token #" << t.kind() << "\n";

   //INVARIANT: max_matching_idx points to an applicable rewrite rule
   env.rw_rule_in_execution_ = min_matching_idx;
   env.rw_rule_in_execution_ip_ = rw_rules_[env.rw_rule_in_execution_].pattern_ + rw_rules_[env.rw_rule_in_execution_].pattern_len_;
   if (rw_rules_[min_matching_idx].pattern_len_ == 0) { t = orig_t;return r;/*no token consumed*/}
   rw_rules_[env.rw_rule_in_execution_].rewind_state() = state_raw_lexer_before_gettoken_local;
   continue;

  }//for
 }//gettoken(Token& t)
};

bool readfile_to_memory(Memory<char>& mem, char* filename );

extern const char *   default_ops[];

