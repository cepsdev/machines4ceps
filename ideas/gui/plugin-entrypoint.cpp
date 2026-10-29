
#include <stdlib.h>
#include <iostream>
#include <ctype.h>
#include <chrono>
#include <sstream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <map>
#include <algorithm>
#include <future>

#include "ceps_ast.hh"
#include "core/include/state_machine_simulation_core.hpp"


#include "raylib.h"
#include "raymath.h"

namespace cepsplugin{
    static Ism4ceps_plugin_interface* plugin_master = nullptr;
    static const std::string version_info = "gui v0.1";
    static constexpr bool print_debug_info{true};
    ceps::ast::node_t plugin_entrypoint(ceps::ast::node_callparameters_t params);
}


std::thread* gui_main_thread {nullptr};

void draw_rect(int x, int y, int w, int h ){
 //DrawRectangle(x,y,w,h,GRAY);
 DrawRectangleRounded(Rectangle{x,y,w,h}, .5f, 32,  GRAY);
}

void draw_rects(int x, int y, int rows, int cols){
 for(int r = 0; r < rows; ++r) 
  for (int c = 0; c < cols; ++c) 
   draw_rect(x + 4 + c*(20+4),y + 4+r*(20+4),20,20);
}

void draw_rects(int x, int y, int rows, int cols, std::vector<bool> status){
 size_t s{};
 for(int r = 0; r < rows; ++r) 
  for (int c = 0; c < cols; ++c,++s) 
   DrawRectangleRounded(Rectangle{x + 4 + c*(20+4),y + 4+r*(20+4),20,20}, .5f, 32,  !status[s] ? LIGHTGRAY : GRAY);
}

void gui_main(){
    InitWindow(800,600,"ceps - gui");
    SetTargetFPS(60);
    std::vector<bool> status;
    status.resize(400);
    status[21] = true;
    for(;!WindowShouldClose();){
        BeginDrawing();
         ClearBackground(RAYWHITE);
         DrawText("ceps-gui",190,200,20,LIGHTGRAY);
         draw_rects(0,0,20,20,status);
        EndDrawing();
    }
    CloseWindow();
}

ceps::ast::node_t cepsplugin::plugin_entrypoint(ceps::ast::node_callparameters_t params){
    using namespace std;
    using namespace ceps::ast;
    using namespace ceps::interpreter;

    gui_main_thread = new thread(gui_main);
    gui_main_thread->detach();
    
    
    

    auto data = get_first_child(params);    
    if (!is<Ast_node_kind::structdef>(data)) return nullptr;
    auto& ceps_struct = *as_struct_ptr(data);
    cout << "cepsplugin::plugin_entrypoint:\n";
    for(auto e : children(ceps_struct)){
        cout <<"\t"<< * e << "\n";
    }
    cout <<"\n\n";
    auto result = mk_struct("result");
    children(*result).push_back(mk_int_node(42));
    return result;
}

extern "C" void init_plugin(IUserdefined_function_registry* smc)
{
  cepsplugin::plugin_master = smc->get_plugin_interface();
  cepsplugin::plugin_master->reg_ceps_phase0plugin("gui", cepsplugin::plugin_entrypoint);
}					
				