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

using namespace std;

struct Sequence_diagram;

struct Scene{
    double width_px = 1920;
    double height_px = 1080;
    double bound_width;
    double bound_height;

    virtual void draw(Sequence_diagram& sd) = 0;
    virtual void set_bounds(double width, double height) = 0;
};


struct Event{
    string title;
    double time;  // 0 <= time <= 1
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

struct Scene_SVG: public Scene{
     ostream* os;
     Scene_SVG(ostream& os):os{&os} {}
     void draw(Sequence_diagram& sd) override;
     void set_bounds(double width, double height) override;
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
}    


void Scene_SVG::draw(Sequence_diagram& sd){
    sd.build_scene(*this);
    *os << "<svg height=\"" << round(height_px) << "\"" << " width=\"" << round(width_px) << "\">";
    *os << "</svg>";
}

void Scene_SVG::set_bounds(double width, double height){
    bound_width = width;
    bound_height = height;
}

int main(int argc, char**argv){
    
    Scene_SVG sc_svg{cout};
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
                 Lane{"State Machine C",{
                                            Event{"Event A",0.4},
                                            Event{"Event B",0.5},
                                            Event{"Event C",0.6}
                                        }
                 }};
    sc_svg.draw(sd);


}