function build_ctrlpanel_from_json(container, descr) {
    let t = 50;
    container.set_name(ctrlpanel_info_get_name(descr));
    container.clear();
    for (let e of descr.content) {
        let info = ctrlpanel_info_get_widget_descr(e);
        
        if (!info[0]) continue;
        let widget = container.addWidget();

        let widget_info = {
            valid: true,
            type: {
                sig: info[1].type == "toggle_signal" || info[1].type == "watch_signal" || info[1].type == "plot_signal",
                ev: info[1].type == "trigger_event",
                script: false
            },
            doc: undefined,
            sig_name: info[1].associated_systemstate,
            ev_name: info[1].associated_event,
            do_plot: info[1].type == "plot_signal",
            watch_only: info[1].type == "watch_signal",
            toggle_sig: info[1].type == "toggle_signal",
            update_sig_automatically: false,
            display: "Decimal"
        };

        //console.log(widget_info);

        if (info[1].style != undefined && info[1].style.width != undefined) $("#" + widget).css("width", info[1].style.width);
        if (info[1].style != undefined && info[1].style.height != undefined) $("#" + widget).css("height", info[1].style.height);

        //setTimeout(() => { save_widget_info(widget, widget_info, true); }, t);
        console.log(widget_info);
        save_widget_info(widget, widget_info, true);
        t += 50;
        //save_widget_info(widget, widget_info, true);
    }
}

function build_ceps_from_control_panel(ctrl_editor_id) {
     return `
ctrlpanel{${ctrlPanelEditor.get_name() ? 'name{"' + ctrlPanelEditor.get_name() + '";};' : ''}`+
    (() => {
        let s = "";
        for (let ch of $("#" + ctrl_editor_id).children()) {
            let winf = get_widget_info($(ch).attr("id"));
            function get_type() {
                if (winf.type.sig) { if (winf.toggle_sig) return "toggle_signal"; if (winf.watch_only) return "watch_signal"; if (winf.do_plot) return "plot_signal"; }
                else if (winf.type.ev) { return "trigger_event"; }
                return undefined;
            }
            function get_assoc_sysstate() { return winf.sig_name; }
            function get_assoc_event() { return winf.ev_name; }
            function get_style() {
                return `width{"${$(ch).css("width")}";};height{"${$(ch).css("height")}";}`; 
            }
            if (winf == undefined) continue;
            if (!$(ch).hasClass("baseWidget")) continue;
            s += `
 widget{
  type{${get_type()};};
  `
                + (get_assoc_sysstate() != undefined ? `associated_systemstate{as_symbol("${get_assoc_sysstate()}", "Systemstate"); };` : "")
                + (get_assoc_event() != undefined ? `associated_event{as_symbol("${get_assoc_event()}", "Event"); };` : "")
                +

  `style{${get_style()};};    
 };` 
               ;
            }
            return s;
    })()+ `
};`;
}

function save_active_control_panel(name) {
    ctrlPanelEditor.set_name(name);
    let s = "ctrlpanels{" + build_ceps_from_control_panel("ctrl_panel_editor") + "};";
    console.log(s);
    simcore_ws.send(`PUSH ${s}`);
}