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
#include "core/include/transform/streamtransform.hpp"



int main(int argc, char *argv[])
{
  /*argc = 3;
  argv = new char* [3];
  argv[0] = "";
  argv[1] ="/home/tomas/projects/sm4ceps/test/cepslexer/a.rules";
  argv[2] = "/home/tomas/projects/sm4ceps/test/cepslexer/a.txt";*/

  if (argc == 1)
    {std::cout << "Usage: streamtransform FILE [FILE...]\n";return 1;}


  std::vector<Memory<char>> file_contents;
  Statefulscanner<Memory<char>,char> scanner;

  size_t ttt = 0;
  for(;default_ops[ttt++];);
  scanner.tokentable().clear_and_read_table(default_ops,--ttt);
  Statefulscanner<Memory<char>,char>::Token t;

  int files = 0;

  for(int i = 1; i < argc; ++i)
  {
	  if (strlen(argv[i]) == 0) continue;
	  if (argv[i][0] == '-' ) continue;
	  ++files;
  }
  file_contents.resize(files);
  int file_idx = 0;


  for(int i = 1; i < argc; ++i)
  {
	  if (strlen(argv[i]) == 0) continue;
	  if (argv[i][0] == '-' )
	  {

	  } else
	  {
		  if(!readfile_to_memory(file_contents[file_idx++] , argv[i] )) {
			  std::cerr << "Cannot open/read '"<< argv[i] <<"'\n"; return 2;
		  }
	  }
  }//for

  scanner.activate_toplevel_onexit_trigger(false);
  for(int i = 0; i < file_contents.size(); ++i)
  {
	  scanner.set_input(file_contents[i]);
	  if (i+1 == file_contents.size()) { scanner.activate_toplevel_onexit_trigger(true);}

	  for(;scanner.gettoken(t);)
	   {


		if (t.kind() == Statefulscanner<Memory<char>,char>::Token::TOK_STR)
	    {
	      std::cout << "\"" <<  t.sval_ << "\"";
	    }
	    else if (t.kind() == Statefulscanner<Memory<char>,char>::Token::TOK_ENDL) std::cout << std::endl;
        else if (t.kind() == Statefulscanner<Memory<char>,char>::Token::TOK_BLANK) std::cout << " ";
        else if (t.kind() == Statefulscanner<Memory<char>,char>::Token::TOK_DOUBLE_QUOTE) std::cout << "\"";
	    else std::cout << t.sval_;
	   }

  }//for
  return 0;
}
