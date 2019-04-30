function build_lang_aware_text_elem(text,current_lang){
    let r = '<span class="lang-aware-text" ';
    for(key in text){
        r+= " lang-"+key+" = \""+text[key]+"\" ";
    }
    let t = text[current_lang];
    r+=">"+t+"</span>";
    return r;
}

function create_main_tab_ctrl(lc){
    let tab_id_ctr = 0;
    let lang_ctxt = lc;
    let infos = [];
    let cur_tab_idx = -1;
    let add_callback = undefined;
    let self = undefined;

    $(".add-a-tab-tab").click(add_tab_tab_hit);

    function add_tab_tab_hit(ev){
       ev.stopImmediatePropagation();
       if (add_callback) add_callback(self,ev);
    }

    return self = {
        reg_add_clbk: function(f){add_callback = f;},
        add_tab : function (title,pre,post){
            if (title == undefined) title = {en:"New Search ("+tab_id_ctr+")",de:"Neue Suche ("+tab_id_ctr+")"};
            if (typeof title == "string") title = {en:title,de:title};
            let q;
            /*$("#main_nav_tabs").append(
                '<li><a class="main-tab-header" data-toggle="tab" id="main-tab-header'+tab_id_ctr.toString()+'" href="#main_tab'+tab_id_ctr.toString()+'">'+
                    build_lang_aware_text_elem(title,lang_ctxt.current)+
                '</a></li>'
            );*/
            if (pre == undefined) pre = "";
            $(".add-a-tab-tab").parent().before(
                '<li><a class="main-tab-header" data-toggle="tab" id="main-tab-header'+tab_id_ctr.toString()+'" href="#main_tab'+tab_id_ctr.toString()+'">'+
                    pre +
                    build_lang_aware_text_elem(title,lang_ctxt.current)+
                '</a></li>');
            let t;infos.push({active_filters:t=get_expert_filters_copy()});t.forEach(function(e){e.selected=false;});
            return tab_id_ctr++;
        },
        add_content : function(idx,content){
            $("#main_tab_content").append(
                '<div id="main_tab'+idx.toString()+'" class="tab-pane xfade xin">'+
                content+
                '</div>'
            );
        },
        set_content : function(idx,content){
            $("#main_tab"+idx).html(
                content
            );
        },
        select: function(idx){
            if (idx == undefined) idx = tab_id_ctr-1;
            $('.nav-tabs a[href="#main_tab'+idx.toString()+'"]').tab('show');
            cur_tab_idx = idx;
        },
        get_infos: function(idx) {return infos[idx];},
        count : function() {return infos.length;},
        cur_tab : function () {return cur_tab_idx;}
    }        
}



