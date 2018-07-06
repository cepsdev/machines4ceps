let layout_info = {
    sm_widget_tile : {
        width:156, 
        height:270,
        outter_width:160, 
        outter_height:300},
    
    sm_minimized : {
        width:156, 
        height:23,
        outter_width:160, 
        outter_height:40}
};

function setup_tile(tile_scale,
                    sm_name,
                    svg_orig_width,
                    svg_orig_height,
                    minimized){
                        
    if (minimized){
        $(`#sm_viz_${normalize_id(sm_name)}`).animate({height:`${layout_info.sm_minimized.outter_height}px`},500,"",
            function(){
                $(`#sm_viz_details_svg_div_${normalize_id(sm_name)}`).html("");
                $(`#sm_viz_details_${normalize_id(sm_name)}`).css("height",0);
                $(`#info_${normalize_id(sm_name)}`).removeClass("sm-details-info");
                $(`#info_${normalize_id(sm_name)}`).html("");        
            });
        return;
    }

    d3_util_scale(d3.select(`#sm_viz_details_${normalize_id(sm_name)}`).select("svg").select("g"),tile_scale,tile_scale);
    $(`#sm_viz_${normalize_id(sm_name)}`).css("height",0);

    $(`#sm_viz_details_${normalize_id(sm_name)}`).css("height",layout_info.sm_widget_tile.height);
    
    $(`#sm_viz_${normalize_id(sm_name)}`).animate({height:`${layout_info.sm_widget_tile.outter_height}px`},500);
    $(`#sm_viz_${normalize_id(sm_name)}`).css("height",layout_info.sm_widget_tile.outter_height);
    $(`#sm_viz_details_${normalize_id(sm_name)}`).css("overflow","hidden");
    $(`#sm_viz_${normalize_id(sm_name)}`).css("overflow","hidden");


    $(`#sm_viz_details_${normalize_id(sm_name)}`).mouseenter(
        function(ev){
          //if (!$(`#sm_viz_${sm_name}`).hasClass("ceps-ui-tile")) return;
            if (tile_scale < 1.0) {
                new_tile_scale = tile_scale * 5.0;
                if (new_tile_scale > 1.0) new_tile_scale = 1.0;
                let svg_containing_div = $(`#sm_viz_details_svg_div_${normalize_id(sm_name)}`);
                d3_util_scale(d3.select(`#sm_viz_details_svg_div_${normalize_id(sm_name)}`).select("svg").select("g"),new_tile_scale,new_tile_scale);
            }                                        
        }
    );

    $(`#sm_viz_details_${normalize_id(sm_name)}`).mouseleave(
        function(ev){
            //if (!$(`#sm_viz_${sm_name}`).hasClass("ceps-ui-tile")) return;
            let sm_viz_container = $(`#sm_viz_details_svg_div_${normalize_id(sm_name)}`);
            d3_util_scale(d3.select(`#sm_viz_details_svg_div_${normalize_id(sm_name)}`).select("svg").select("g"),tile_scale,tile_scale);
            sm_viz_container.css("top",0);
            sm_viz_container.css("left",0);
            
        }
    );                                
                                
    $(`#sm_viz_details_${normalize_id(sm_name)}`).mousemove(
        function(ev){
            //if (!$(`#sm_viz_${sm_name}`).hasClass("ceps-ui-tile")) return;
            let sm_viz_container = $(`#sm_viz_details_svg_div_${normalize_id(sm_name)}`);
            let w_pic = svg_orig_width * new_tile_scale;
            let h_pic = svg_orig_height * new_tile_scale;

            let w_parent = $(`#sm_viz_details_${normalize_id(sm_name)}`).width();
            let h_parent = $(`#sm_viz_details_${normalize_id(sm_name)}`).height();

            let m_pos_y = ev.pageY - $(`#sm_viz_details_${normalize_id(sm_name)}`).offset().top;
            let m_pos_x = ev.pageX - $(`#sm_viz_details_${normalize_id(sm_name)}`).offset().left;

            if (m_pos_y + 4 > h_parent) m_pos_y = h_parent;
            if (m_pos_x + 4 > w_parent) m_pos_x = w_parent;
                                        
            if (m_pos_y > h_parent) m_pos_y = h_parent;
            if (m_pos_x > w_parent) m_pos_x = w_parent;
                                      
            let sy = m_pos_y / h_parent; if (sy > 1.0) sy = 1.0;
            let sx = m_pos_x / w_parent; if (sx > 1.0) sx = 1.0;
                                      
            let pos_in_pic_y = sy * h_pic ;// * h_pic;
            let pos_in_pic_x = sx * w_pic;
                                        
            let dy = -(pos_in_pic_y - m_pos_y);
            if (dy + h_pic < h_parent) dy = h_parent - h_pic;
            if (dy >= 0) dy = 0;
            sm_viz_container.css("position","relative");

            sm_viz_container.css("top",  dy);
                                        
            let dx = -(pos_in_pic_x - m_pos_x);
            if (dx + w_pic < w_parent) dx = w_parent - w_pic;
            if (dx >= 0) dx = 0;
            sm_viz_container.css("left",  dx);                                  
        }
    );
}


function handle_tile_click(sm_name){


    let sm_viz = $(`#sm_viz_${normalize_id(sm_name)}`);$(sm_viz).removeClass("details-active");
    let sm_details = $(`#sm_viz_details_${normalize_id(sm_name)}`);
    let svg_containing_div = $(`#sm_viz_details_svg_div_${normalize_id(sm_name)}`);
    let svg_info = get_svg_infos(sm_name);
    let show_details_btn = $(`#show_details_btn_${normalize_id(sm_name)}`);
    let sm_info = $(`#info_${normalize_id(sm_name)}`);
    let add_svg = true;
    if ($(show_details_btn).hasClass("details-show-btn")){
        $(show_details_btn).removeClass("details-show-btn");
        $(show_details_btn).addClass("details-hide-btn");
        $(show_details_btn).css("animation-name","show-details-anim");
        $(show_details_btn).css("animation-duration","0.5s");
        $(show_details_btn).css("transform","rotate(-180deg)");
        $(sm_viz).addClass("details-active");             
    } else if ($(show_details_btn).hasClass("details-hide-btn")){
        add_svg = false;
        $(show_details_btn).removeClass("details-hide-btn");
        $(show_details_btn).addClass("details-show-btn");
        $(show_details_btn).css("animation-name","hide-details-anim");
        $(show_details_btn).css("animation-duration","0.5s");
        $(show_details_btn).css("transform","rotate(0deg)"); 
    }

    if (add_svg){
        $(svg_containing_div).html(svg_info.data);
        //Scale to fit
        let svg_orig_height = parseFloat(d3.select(`#sm_viz_details_svg_div_${normalize_id(sm_name)}`).select("svg").style("height"));
        let svg_orig_width = parseFloat(d3.select(`#sm_viz_details_svg_div_${normalize_id(sm_name)}`).select("svg").style("width"));
        let svg_scale_y = layout_info.sm_widget_tile.height / svg_orig_height;
        let svg_scale_x = layout_info.sm_widget_tile.width / svg_orig_width;
        if (svg_scale_y > svg_scale_x) svg_scale_y = svg_scale_x;
        if (svg_scale_x > svg_scale_y) svg_scale_x = svg_scale_y;
        if (svg_scale_x >= 1.0) {svg_scale_x = 1.0;svg_scale_y = 1.0;}
        if (svg_scale_y >= 1.0) {svg_scale_x = 1.0;svg_scale_y = 1.0;}
        let tile_scale = svg_scale_x; if (tile_scale < 0.1)tile_scale = 0.1;
        let new_tile_scale = tile_scale;
        setup_tile(tile_scale,sm_name,svg_orig_width,svg_orig_height,false);
    } else {
        //$(svg_containing_div).html("");
        setup_tile(0,sm_name,0,0,true);
    }
}


