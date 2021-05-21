/*
Copyright 2021 Tomas Prerovsky (cepsdev@hotmail.com).

Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <memory>

using namespace std;

struct Sequence_diagram;
struct Scene;


struct Scene_element{
    enum class Anchor{Midpoint,TopLeft};
    Anchor anchor{Anchor::Midpoint};
    double anchor_x{};
    double anchor_y{};
    Scene_element() = default;
    Scene_element(Anchor anchor,double anchor_x, double anchor_y ):anchor{anchor}, anchor_x{anchor_x}, anchor_y{anchor_y} {}
    virtual void draw(Scene&) = 0;
};

struct Scene{
    double width_px = 1920;
    double height_px = 1080;
    double bound_width;
    double bound_height;
    vector<shared_ptr<Scene_element>> scene_elements;

    ostream* os;
    Scene(ostream& os):os{&os} {}

    virtual void draw(Sequence_diagram& sd);
    virtual void set_bounds(double width, double height);
};



struct Scene_element_textbox:public Scene_element{
    string title;
    double width;
    double height;
    Scene_element_textbox() = default;
    Scene_element_textbox(string title):title{title}{}
    Scene_element_textbox(string title,
                          Anchor anchor,
                          double anchor_x, 
                          double anchor_y,
                          double width = 0.1,
                          double height = 0.1):Scene_element{anchor,anchor_x,anchor_y},title{title},width{width},height{height}{}
    void draw(Scene&) override;
};

void Scene_element_textbox::draw(Scene& scene){

    if (anchor == Anchor::Midpoint){
        *scene.os << "<rect fill=\"none\" stroke=\"black\" x=\""<<anchor_x*scene.width_px-(width/2.0)*scene.width_px<<"\" y=\""<<anchor_y*scene.height_px-(height/2.0)*scene.height_px<<"\" width=\""<<width*scene.width_px<<"\" height=\""<<height*scene.height_px<<"\"> </rect>";    
    }

    *scene.os << 
    R"(
        <text text-anchor="middle" alignment-baseline="central">
            <tspan x=")"<< anchor_x*scene.width_px << R"(" y=")" << anchor_y*scene.height_px << R"(">)" << title << R"(</tspan>
        </text>
    )";
}


struct Event{
    string title;
    double time;  // 0 <= time <= 1
    bool in = false;
    bool out = false;
    Event() = default;
    Event(Event const &) = default;
    Event(string title,double time): title{title},time{time}{}
    double get_time() const {return time;}
};

struct Lane{
    string title;
    vector<Event> events;
    Lane() = default;
    Lane(Lane const&) = default;
    Lane(string title):title{title}{}
    Lane(string title,vector<Event> events):title{title},events{events}{}
    double get_max_time() const ;
};


struct Sequence_diagram{
    vector<Lane> lanes;
    void build_scene(Scene& scene);
    double get_max_time() const;
};


double Lane::get_max_time()const{
    double mt = 0.0;
    for(auto const & l: events)
        if (mt < l.get_time()) mt = l.get_time();
    return mt;
}

double Sequence_diagram::get_max_time() const {
    double mt = 0.0;
    for(auto const & l: lanes)
        if (auto v= l.get_max_time(); v > mt ) mt = v;
    return mt;
}


void Sequence_diagram::build_scene(Scene& scene){ 
    scene.set_bounds(1.0,get_max_time());
    auto num_of_top_down_lines = lanes.size();
    if (num_of_top_down_lines == 0) return;
    for(size_t i = 0; i < lanes.size();++i){
        auto const & cur_lane = lanes[i];
        scene.scene_elements.push_back  ( make_shared<Scene_element_textbox>(
                                                Scene_element_textbox{cur_lane.title,
                                                Scene_element::Anchor::Midpoint,
                                                i * 1.0/num_of_top_down_lines,
                                                0.1}
                                            )
                                    );
    }
}    


void Scene::draw(Sequence_diagram& sd){
    sd.build_scene(*this);
    *os << "<svg height=\"" << round(height_px) << "\"" << " width=\"" << round(width_px) << "\">";
    for(auto obj:scene_elements)
     obj->draw(*this);
    *os << "</svg>";
}

void Scene::set_bounds(double width, double height){
    bound_width = width;
    bound_height = height;
}

int main(int argc, char**argv){
    
    Scene sc_svg{cout};
    Sequence_diagram sd;
    sd.lanes = vector<Lane>{ Lane{"State Machine A",{
                                            Event{"Event A",0.01},
                                            Event{"Event B",0.1},
                                            Event{"Event C",0.2}
                                        }
                     }, 
                 Lane{"State Machine B",{
                                            Event{"Event A",0.01},
                                            Event{"Event B",0.1},
                                            Event{"Event C",0.2}
                                        }
                    }, 
                 Lane{"State Machine C Blabla gaga hjhj hjhjhj hjhjh",{
                                            Event{"Event A",0.4},
                                            Event{"Event B",0.5},
                                            Event{"Event C",0.6}
                                        }
                 }, 
                 Lane{"State Machine D",{
                                            Event{"Event A",0.4},
                                            Event{"Event B",0.5},
                                            Event{"Event C",0.6}
                                        }
                 }, 
                 Lane{"State Machine E",{
                                            Event{"Event A",0.4},
                                            Event{"Event B",0.5},
                                            Event{"Event C",0.6}
                                        }
                 }, 
                 Lane{"State Machine F",{
                                            Event{"Event A",0.4},
                                            Event{"Event B",0.5},
                                            Event{"Event C",0.6}
                                        }
                 }, 
                 Lane{"State Machine G",{
                                            Event{"Event A",0.4},
                                            Event{"Event B",0.5},
                                            Event{"Event C",0.6}
                                        }
                 }
                 };

    cout << "<!DOCTYPE html><html><body>\n";

    sc_svg.draw(sd);

    cout << "</body></html>";


}