

// --cppgen_statemachines 
#include <iostream>
#include <string>
#include <algorithm>
#include <map>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include "core/include/state_machine_simulation_core_reg_fun.hpp"


void create_statemachine1(Ism4ceps_plugin_interface*);
void create_statemachine2(Ism4ceps_plugin_interface*);
void create_statemachine3(Ism4ceps_plugin_interface*);
void create_statemachine4(Ism4ceps_plugin_interface*);
void create_statemachine5(Ism4ceps_plugin_interface*);
void create_statemachine6(Ism4ceps_plugin_interface*);
void create_statemachine7(Ism4ceps_plugin_interface*);
void create_statemachine8(Ism4ceps_plugin_interface*);
void create_statemachine9(Ism4ceps_plugin_interface*);
void create_statemachine10(Ism4ceps_plugin_interface*);
void create_statemachine11(Ism4ceps_plugin_interface*);
void create_statemachine12(Ism4ceps_plugin_interface*);
void create_statemachine13(Ism4ceps_plugin_interface*);
void create_statemachine14(Ism4ceps_plugin_interface*);
void create_statemachine15(Ism4ceps_plugin_interface*);
void create_statemachine16(Ism4ceps_plugin_interface*);
void create_statemachine17(Ism4ceps_plugin_interface*);
void create_statemachine18(Ism4ceps_plugin_interface*);
void create_statemachine19(Ism4ceps_plugin_interface*);

void create_statemachines(IUserdefined_function_registry* smc){auto smcore_interface=smc->get_plugin_interface();
smcore_interface->drop_all_sms();
 smcore_interface->create_sm("S1", "S1", 1 ,1);
 smcore_interface->create_sm("S1a", "S1.S1a", 2 ,2);
 smcore_interface->create_sm("S1aa", "S1.S1a.S1aa", 3 ,3);
 smcore_interface->create_sm("S1ab", "S1.S1a.S1ab", 3 ,4);
 smcore_interface->create_sm("S1ac", "S1.S1a.S1ac", 3 ,5);
 smcore_interface->create_sm("S1b", "S1.S1b", 2 ,6);
 smcore_interface->create_sm("S1ba", "S1.S1b.S1ba", 3 ,7);
 smcore_interface->create_sm("S1bb", "S1.S1b.S1bb", 3 ,8);
 smcore_interface->create_sm("S1bc", "S1.S1b.S1bc", 3 ,9);
 smcore_interface->create_sm("S1c", "S1.S1c", 2 ,10);
 smcore_interface->create_sm("S1ca", "S1.S1c.S1ca", 3 ,11);
 smcore_interface->create_sm("S1cb", "S1.S1c.S1cb", 3 ,12);
 smcore_interface->create_sm("S1cc", "S1.S1c.S1cc", 3 ,13);
 smcore_interface->create_sm("S2", "S2", 1 ,0);
 smcore_interface->create_sm("S3", "S3", 1 ,14);
 smcore_interface->create_sm("S1", "S3.S1", 2 ,15);
 smcore_interface->create_sm("S2", "S3.S2", 2 ,16);
 smcore_interface->create_sm("thread_1", "S3.thread_1", 2 ,17);
 smcore_interface->create_sm("thread_2", "S3.thread_2", 2 ,18);
 create_statemachine1(smcore_interface);
 create_statemachine2(smcore_interface);
 create_statemachine3(smcore_interface);
 create_statemachine4(smcore_interface);
 create_statemachine5(smcore_interface);
 create_statemachine6(smcore_interface);
 create_statemachine7(smcore_interface);
 create_statemachine8(smcore_interface);
 create_statemachine9(smcore_interface);
 create_statemachine10(smcore_interface);
 create_statemachine11(smcore_interface);
 create_statemachine12(smcore_interface);
 create_statemachine13(smcore_interface);
 create_statemachine14(smcore_interface);
 create_statemachine15(smcore_interface);
 create_statemachine16(smcore_interface);
 create_statemachine17(smcore_interface);
 create_statemachine18(smcore_interface);
 create_statemachine19(smcore_interface);
 }

 void create_statemachine1(Ism4ceps_plugin_interface* smcore_interface){
 
// S1

 {auto t=smcore_interface->get_sm("S1");
 smcore_interface->sm_set_misc_attributes(t,0,0,1,0,-1);
 
// S1.Initial -> S1.S1a

 smcore_interface->sm_add_transition(t,-1, "", nullptr);
 smcore_interface->sm_transition_set_from(t,-1, "Initial", 0 ,smcore_interface->get_sm("S1"), nullptr, 0, 0);
 smcore_interface->sm_transition_set_to(t,-1, "S1a", 1 ,smcore_interface->get_sm("S1.S1a"), nullptr, 0, 0);
 smcore_interface->sm_transition_add_ev(t,-1,"A",0);
 smcore_interface->sm_transition_add_action(t,-1,"t1", nullptr);
 
// S1.Initial -> S1.state_a

 smcore_interface->sm_add_transition(t,-1, "", nullptr);
 smcore_interface->sm_transition_set_from(t,-1, "Initial", 0 ,smcore_interface->get_sm("S1"), nullptr, 0, 0);
 smcore_interface->sm_transition_set_to(t,-1, "state_a", 0 ,smcore_interface->get_sm("S1"), nullptr, 0, 0);
 smcore_interface->sm_transition_add_ev(t,-1,"A",0);
 smcore_interface->sm_transition_add_action(t,-1,"t4", nullptr);
 
// S1.state_a -> S1.S2

 smcore_interface->sm_add_transition(t,-1, "", nullptr);
 smcore_interface->sm_transition_set_from(t,-1, "state_a", 0 ,smcore_interface->get_sm("S1"), nullptr, 0, 0);
 smcore_interface->sm_transition_set_to(t,-1, "S2", 1 ,smcore_interface->get_sm("S2"), nullptr, 0, 0);
 smcore_interface->sm_transition_add_ev(t,-1,"F",0);
 smcore_interface->sm_transition_add_action(t,-1,"t3", nullptr);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S1"), nullptr, 0, 0);
 smcore_interface->sm_add_state(t, "state_a", 0 ,smcore_interface->get_sm("S1"), nullptr, 0, 0);
 
// S1 Actions

 smcore_interface->sm_transition_add_action(t,"t1", nullptr);
 smcore_interface->sm_transition_add_action(t,"t2", nullptr);
 smcore_interface->sm_transition_add_action(t,"t3", nullptr);
 smcore_interface->sm_transition_add_action(t,"t4", nullptr);
 smcore_interface->sm_transition_add_action(t,"on_enter",smcore_interface->get_sm("S1"));
 smcore_interface->sm_transition_add_action(t,"on_exit",smcore_interface->get_sm("S1"));
 
// Children

 smcore_interface->sm_add_child(t,smcore_interface->get_sm("S1.S1a"));
 smcore_interface->sm_add_child(t,smcore_interface->get_sm("S1.S1b"));
 smcore_interface->sm_add_child(t,smcore_interface->get_sm("S1.S1c"));
}}
 void create_statemachine2(Ism4ceps_plugin_interface* smcore_interface){
 
// S1.S1a

 {auto t=smcore_interface->get_sm("S1.S1a");
 smcore_interface->sm_set_misc_attributes(t,0,0,1,0,-1);
 
// S1.S1a.Initial -> S1.S1a.S1aa

 smcore_interface->sm_add_transition(t,-1, "", nullptr);
 smcore_interface->sm_transition_set_from(t,-1, "Initial", 0 ,smcore_interface->get_sm("S1.S1a"), nullptr, 0, 0);
 smcore_interface->sm_transition_set_to(t,-1, "S1aa", 1 ,smcore_interface->get_sm("S1.S1a.S1aa"), nullptr, 0, 0);
 smcore_interface->sm_transition_add_ev(t,-1,"B",0);
 smcore_interface->sm_transition_add_action(t,-1,"t1", nullptr);
 
// S1.S1a.Initial -> S1.S1a.Initial

 smcore_interface->sm_add_transition(t,-1, "", nullptr);
 smcore_interface->sm_transition_set_from(t,-1, "Initial", 0 ,smcore_interface->get_sm("S1.S1a"), nullptr, 0, 0);
 smcore_interface->sm_transition_set_to(t,-1, "Initial", 0 ,smcore_interface->get_sm("S1.S1a"), nullptr, 0, 0);
 smcore_interface->sm_transition_add_ev(t,-1,"B",0);
 smcore_interface->sm_transition_add_action(t,-1,"t2", nullptr);
 
// S1.S1a.S1aa -> S1.S1a.S1ab

 smcore_interface->sm_add_transition(t,-1, "", nullptr);
 smcore_interface->sm_transition_set_from(t,-1, "S1aa", 1 ,smcore_interface->get_sm("S1.S1a.S1aa"), nullptr, 0, 0);
 smcore_interface->sm_transition_set_to(t,-1, "S1ab", 1 ,smcore_interface->get_sm("S1.S1a.S1ab"), nullptr, 0, 0);
 smcore_interface->sm_transition_add_ev(t,-1,"C",0);
 smcore_interface->sm_transition_add_action(t,-1,"t3", nullptr);
 
// S1.S1a.S1ab -> S1.S1a.S1ac

 smcore_interface->sm_add_transition(t,-1, "", nullptr);
 smcore_interface->sm_transition_set_from(t,-1, "S1ab", 1 ,smcore_interface->get_sm("S1.S1a.S1ab"), nullptr, 0, 0);
 smcore_interface->sm_transition_set_to(t,-1, "S1ac", 1 ,smcore_interface->get_sm("S1.S1a.S1ac"), nullptr, 0, 0);
 smcore_interface->sm_transition_add_ev(t,-1,"D",0);
 smcore_interface->sm_transition_add_action(t,-1,"t4", nullptr);
 
// S1.S1a.S1ac -> S1.S1a.S1aa

 smcore_interface->sm_add_transition(t,-1, "", nullptr);
 smcore_interface->sm_transition_set_from(t,-1, "S1ac", 1 ,smcore_interface->get_sm("S1.S1a.S1ac"), nullptr, 0, 0);
 smcore_interface->sm_transition_set_to(t,-1, "S1aa", 1 ,smcore_interface->get_sm("S1.S1a.S1aa"), nullptr, 0, 0);
 smcore_interface->sm_transition_add_ev(t,-1,"E",0);
 smcore_interface->sm_transition_add_action(t,-1,"t5", nullptr);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S1.S1a"), nullptr, 0, 0);
 
// S1.S1a Actions

 smcore_interface->sm_transition_add_action(t,"t1", nullptr);
 smcore_interface->sm_transition_add_action(t,"t2", nullptr);
 smcore_interface->sm_transition_add_action(t,"t3", nullptr);
 smcore_interface->sm_transition_add_action(t,"t4", nullptr);
 smcore_interface->sm_transition_add_action(t,"t5", nullptr);
 smcore_interface->sm_transition_add_action(t,"on_enter",smcore_interface->get_sm("S1.S1a"));
 smcore_interface->sm_transition_add_action(t,"on_exit",smcore_interface->get_sm("S1.S1a"));
 
// Children

 smcore_interface->sm_add_child(t,smcore_interface->get_sm("S1.S1a.S1aa"));
 smcore_interface->sm_add_child(t,smcore_interface->get_sm("S1.S1a.S1ab"));
 smcore_interface->sm_add_child(t,smcore_interface->get_sm("S1.S1a.S1ac"));
 smcore_interface->sm_set_parent(t,smcore_interface->get_sm("S1"));}}
 void create_statemachine3(Ism4ceps_plugin_interface* smcore_interface){
 
// S1.S1a.S1aa

 {auto t=smcore_interface->get_sm("S1.S1a.S1aa");
 smcore_interface->sm_set_misc_attributes(t,0,0,1,0,-1);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S1.S1a.S1aa"), nullptr, 0, 0);
 
// S1.S1a.S1aa Actions

 smcore_interface->sm_transition_add_action(t,"on_enter",smcore_interface->get_sm("S1.S1a.S1aa"));
 smcore_interface->sm_transition_add_action(t,"on_exit",smcore_interface->get_sm("S1.S1a.S1aa"));
 smcore_interface->sm_set_parent(t,smcore_interface->get_sm("S1.S1a"));}}
 void create_statemachine4(Ism4ceps_plugin_interface* smcore_interface){
 
// S1.S1a.S1ab

 {auto t=smcore_interface->get_sm("S1.S1a.S1ab");
 smcore_interface->sm_set_misc_attributes(t,0,0,1,0,-1);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S1.S1a.S1ab"), nullptr, 0, 0);
 
// S1.S1a.S1ab Actions

 smcore_interface->sm_transition_add_action(t,"on_enter",smcore_interface->get_sm("S1.S1a.S1ab"));
 smcore_interface->sm_transition_add_action(t,"on_exit",smcore_interface->get_sm("S1.S1a.S1ab"));
 smcore_interface->sm_set_parent(t,smcore_interface->get_sm("S1.S1a"));}}
 void create_statemachine5(Ism4ceps_plugin_interface* smcore_interface){
 
// S1.S1a.S1ac

 {auto t=smcore_interface->get_sm("S1.S1a.S1ac");
 smcore_interface->sm_set_misc_attributes(t,0,0,1,0,-1);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S1.S1a.S1ac"), nullptr, 0, 0);
 
// S1.S1a.S1ac Actions

 smcore_interface->sm_transition_add_action(t,"on_enter",smcore_interface->get_sm("S1.S1a.S1ac"));
 smcore_interface->sm_transition_add_action(t,"on_exit",smcore_interface->get_sm("S1.S1a.S1ac"));
 smcore_interface->sm_set_parent(t,smcore_interface->get_sm("S1.S1a"));}}
 void create_statemachine6(Ism4ceps_plugin_interface* smcore_interface){
 
// S1.S1b

 {auto t=smcore_interface->get_sm("S1.S1b");
 smcore_interface->sm_set_misc_attributes(t,0,0,1,0,-1);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S1.S1b"), nullptr, 0, 0);
 
// Children

 smcore_interface->sm_add_child(t,smcore_interface->get_sm("S1.S1b.S1ba"));
 smcore_interface->sm_add_child(t,smcore_interface->get_sm("S1.S1b.S1bb"));
 smcore_interface->sm_add_child(t,smcore_interface->get_sm("S1.S1b.S1bc"));
 smcore_interface->sm_set_parent(t,smcore_interface->get_sm("S1"));}}
 void create_statemachine7(Ism4ceps_plugin_interface* smcore_interface){
 
// S1.S1b.S1ba

 {auto t=smcore_interface->get_sm("S1.S1b.S1ba");
 smcore_interface->sm_set_misc_attributes(t,0,0,1,0,-1);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S1.S1b.S1ba"), nullptr, 0, 0);
 smcore_interface->sm_set_parent(t,smcore_interface->get_sm("S1.S1b"));}}
 void create_statemachine8(Ism4ceps_plugin_interface* smcore_interface){
 
// S1.S1b.S1bb

 {auto t=smcore_interface->get_sm("S1.S1b.S1bb");
 smcore_interface->sm_set_misc_attributes(t,0,0,1,0,-1);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S1.S1b.S1bb"), nullptr, 0, 0);
 smcore_interface->sm_set_parent(t,smcore_interface->get_sm("S1.S1b"));}}
 void create_statemachine9(Ism4ceps_plugin_interface* smcore_interface){
 
// S1.S1b.S1bc

 {auto t=smcore_interface->get_sm("S1.S1b.S1bc");
 smcore_interface->sm_set_misc_attributes(t,0,0,1,0,-1);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S1.S1b.S1bc"), nullptr, 0, 0);
 smcore_interface->sm_set_parent(t,smcore_interface->get_sm("S1.S1b"));}}
 void create_statemachine10(Ism4ceps_plugin_interface* smcore_interface){
 
// S1.S1c

 {auto t=smcore_interface->get_sm("S1.S1c");
 smcore_interface->sm_set_misc_attributes(t,0,0,1,0,-1);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S1.S1c"), nullptr, 0, 0);
 
// Children

 smcore_interface->sm_add_child(t,smcore_interface->get_sm("S1.S1c.S1ca"));
 smcore_interface->sm_add_child(t,smcore_interface->get_sm("S1.S1c.S1cb"));
 smcore_interface->sm_add_child(t,smcore_interface->get_sm("S1.S1c.S1cc"));
 smcore_interface->sm_set_parent(t,smcore_interface->get_sm("S1"));}}
 void create_statemachine11(Ism4ceps_plugin_interface* smcore_interface){
 
// S1.S1c.S1ca

 {auto t=smcore_interface->get_sm("S1.S1c.S1ca");
 smcore_interface->sm_set_misc_attributes(t,0,0,1,0,-1);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S1.S1c.S1ca"), nullptr, 0, 0);
 smcore_interface->sm_set_parent(t,smcore_interface->get_sm("S1.S1c"));}}
 void create_statemachine12(Ism4ceps_plugin_interface* smcore_interface){
 
// S1.S1c.S1cb

 {auto t=smcore_interface->get_sm("S1.S1c.S1cb");
 smcore_interface->sm_set_misc_attributes(t,0,0,1,0,-1);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S1.S1c.S1cb"), nullptr, 0, 0);
 smcore_interface->sm_set_parent(t,smcore_interface->get_sm("S1.S1c"));}}
 void create_statemachine13(Ism4ceps_plugin_interface* smcore_interface){
 
// S1.S1c.S1cc

 {auto t=smcore_interface->get_sm("S1.S1c.S1cc");
 smcore_interface->sm_set_misc_attributes(t,0,0,1,0,-1);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S1.S1c.S1cc"), nullptr, 0, 0);
 smcore_interface->sm_set_parent(t,smcore_interface->get_sm("S1.S1c"));}}
 void create_statemachine14(Ism4ceps_plugin_interface* smcore_interface){
 
// S2

 {auto t=smcore_interface->get_sm("S2");
 smcore_interface->sm_set_misc_attributes(t,0,0,1,0,-1);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S2"), nullptr, 0, 0);
 
// S2 Actions

 smcore_interface->sm_transition_add_action(t,"on_enter",smcore_interface->get_sm("S2"));
 smcore_interface->sm_transition_add_action(t,"on_exit",smcore_interface->get_sm("S2"));
}}
 void create_statemachine15(Ism4ceps_plugin_interface* smcore_interface){
 
// S3

 {auto t=smcore_interface->get_sm("S3");
 smcore_interface->sm_set_misc_attributes(t,0,1,1,1,-1);
 
// S3.Initial -> S3.S1

 smcore_interface->sm_add_transition(t,-1, "", nullptr);
 smcore_interface->sm_transition_set_from(t,-1, "Initial", 0 ,smcore_interface->get_sm("S3"), nullptr, 0, 0);
 smcore_interface->sm_transition_set_to(t,-1, "S1", 1 ,smcore_interface->get_sm("S3.S1"), nullptr, 0, 0);
 
// S3.S1 -> S3.S2

 smcore_interface->sm_add_transition(t,-1, "", nullptr);
 smcore_interface->sm_transition_set_from(t,-1, "S1", 1 ,smcore_interface->get_sm("S3.S1"), nullptr, 0, 0);
 smcore_interface->sm_transition_set_to(t,-1, "S2", 1 ,smcore_interface->get_sm("S3.S2"), nullptr, 0, 0);
 smcore_interface->sm_transition_add_ev(t,-1,"D",0);
 smcore_interface->sm_transition_add_action(t,-1,"a1", nullptr);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S3"), nullptr, 0, 0);
 smcore_interface->sm_add_state(t, "a", 0 ,smcore_interface->get_sm("S3"), nullptr, 0, 0);
 smcore_interface->sm_add_state(t, "b", 0 ,smcore_interface->get_sm("S3"), nullptr, 0, 0);
 smcore_interface->sm_add_state(t, "c", 0 ,smcore_interface->get_sm("S3"), nullptr, 0, 0);
 smcore_interface->sm_add_state(t, "joinstate", 0 ,smcore_interface->get_sm("S3"), nullptr, 0, 0);
 smcore_interface->sm_set_join_state(t, "joinstate", 0 ,smcore_interface->get_sm("S3"), nullptr, 0, 0);
 
// S3 Actions

 smcore_interface->sm_transition_add_action(t,"a1", nullptr);
 
// Children

 smcore_interface->sm_add_child(t,smcore_interface->get_sm("S3.S1"));
 smcore_interface->sm_add_child(t,smcore_interface->get_sm("S3.S2"));
 smcore_interface->sm_add_child(t,smcore_interface->get_sm("S3.thread_1"));
 smcore_interface->sm_add_child(t,smcore_interface->get_sm("S3.thread_2"));
}}
 void create_statemachine16(Ism4ceps_plugin_interface* smcore_interface){
 
// S3.S1

 {auto t=smcore_interface->get_sm("S3.S1");
 smcore_interface->sm_set_misc_attributes(t,0,0,1,0,-1);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S3.S1"), nullptr, 0, 0);
 smcore_interface->sm_set_parent(t,smcore_interface->get_sm("S3"));}}
 void create_statemachine17(Ism4ceps_plugin_interface* smcore_interface){
 
// S3.S2

 {auto t=smcore_interface->get_sm("S3.S2");
 smcore_interface->sm_set_misc_attributes(t,0,0,1,0,-1);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S3.S2"), nullptr, 0, 0);
 smcore_interface->sm_set_parent(t,smcore_interface->get_sm("S3"));}}
 void create_statemachine18(Ism4ceps_plugin_interface* smcore_interface){
 
// S3.thread_1

 {auto t=smcore_interface->get_sm("S3.thread_1");
 smcore_interface->sm_set_misc_attributes(t,1,0,1,0,-1);
 
// S3.thread_1.Initial -> S3.thread_1.a

 smcore_interface->sm_add_transition(t,-1, "", nullptr);
 smcore_interface->sm_transition_set_from(t,-1, "Initial", 0 ,smcore_interface->get_sm("S3.thread_1"), nullptr, 0, 0);
 smcore_interface->sm_transition_set_to(t,-1, "a", 0 ,smcore_interface->get_sm("S3.thread_1"), nullptr, 0, 0);
 
// S3.thread_1.a -> S3.thread_1.Final

 smcore_interface->sm_add_transition(t,-1, "", nullptr);
 smcore_interface->sm_transition_set_from(t,-1, "a", 0 ,smcore_interface->get_sm("S3.thread_1"), nullptr, 0, 0);
 smcore_interface->sm_transition_set_to(t,-1, "Final", 0 ,smcore_interface->get_sm("S3.thread_1"), nullptr, 0, 0);
 smcore_interface->sm_transition_add_ev(t,-1,"E",0);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S3.thread_1"), nullptr, 0, 0);
 smcore_interface->sm_add_state(t, "Final", 0 ,smcore_interface->get_sm("S3.thread_1"), nullptr, 0, 0);
 smcore_interface->sm_add_state(t, "a", 0 ,smcore_interface->get_sm("S3.thread_1"), nullptr, 0, 0);
 smcore_interface->sm_add_state(t, "b", 0 ,smcore_interface->get_sm("S3.thread_1"), nullptr, 0, 0);
 smcore_interface->sm_add_state(t, "c", 0 ,smcore_interface->get_sm("S3.thread_1"), nullptr, 0, 0);
 smcore_interface->sm_set_parent(t,smcore_interface->get_sm("S3"));}}
 void create_statemachine19(Ism4ceps_plugin_interface* smcore_interface){
 
// S3.thread_2

 {auto t=smcore_interface->get_sm("S3.thread_2");
 smcore_interface->sm_set_misc_attributes(t,1,0,1,0,-1);
 
// S3.thread_2.Initial -> S3.thread_2.b

 smcore_interface->sm_add_transition(t,-1, "", nullptr);
 smcore_interface->sm_transition_set_from(t,-1, "Initial", 0 ,smcore_interface->get_sm("S3.thread_2"), nullptr, 0, 0);
 smcore_interface->sm_transition_set_to(t,-1, "b", 0 ,smcore_interface->get_sm("S3.thread_2"), nullptr, 0, 0);
 
// S3.thread_2.b -> S3.thread_2.Final

 smcore_interface->sm_add_transition(t,-1, "", nullptr);
 smcore_interface->sm_transition_set_from(t,-1, "b", 0 ,smcore_interface->get_sm("S3.thread_2"), nullptr, 0, 0);
 smcore_interface->sm_transition_set_to(t,-1, "Final", 0 ,smcore_interface->get_sm("S3.thread_2"), nullptr, 0, 0);
 smcore_interface->sm_transition_add_ev(t,-1,"F",0);
 
// States

 smcore_interface->sm_add_state(t, "Initial", 0 ,smcore_interface->get_sm("S3.thread_2"), nullptr, 0, 0);
 smcore_interface->sm_add_state(t, "Final", 0 ,smcore_interface->get_sm("S3.thread_2"), nullptr, 0, 0);
 smcore_interface->sm_add_state(t, "a", 0 ,smcore_interface->get_sm("S3.thread_2"), nullptr, 0, 0);
 smcore_interface->sm_add_state(t, "b", 0 ,smcore_interface->get_sm("S3.thread_2"), nullptr, 0, 0);
 smcore_interface->sm_add_state(t, "c", 0 ,smcore_interface->get_sm("S3.thread_2"), nullptr, 0, 0);
 smcore_interface->sm_set_parent(t,smcore_interface->get_sm("S3"));}}


