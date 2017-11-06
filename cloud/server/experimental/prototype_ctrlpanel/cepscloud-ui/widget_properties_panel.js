    
        var prop_panel = undefined;
        var prop_panel_header = undefined;
        var widget_infos = [];
        const ACC_TAB_NONE = -1;
        const ACC_TAB_TOGGLE_SIGNAL = 0;
        const ACC_TAB_WATCH_SIGNAL  = 1;
        const ACC_TAB_PLOT          = 2;
        var widget_panel_accordion_active = ACC_TAB_NONE;
        var widget_panel_accordion_active_last = widget_panel_accordion_active;
        var widget_panel_html = 
`   <div id="widget_properties_panel" class="panel panel-primary prop-panel" style="display:none;width:400px;">
        <div class="panel-heading" id="widget_properties_panel_header">
            Widget Properties
            <button type="button" class="close" aria-label="Close" onclick="close_widget_properties_panel();">
                <span aria-hidden="true">&times;</span>
            </button>
        </div>
        <div class="panel-body">             
                <div class="input-group" style="width:100%;">
                        <select id="select_widget_type" class="form-control widget-properties-panel-input" disabled>
                            <option value="Choose ..." >Choose ...</option>
                            <option value="Modify/View Signal">Modify/View Signal</option>
                            <option value="Trigger Event">Trigger Event</option>
                            <!--option value="Run Script">Run Script</option-->
                        </select>
                </div>
                <hr />
                <div id="details_for_sel_1" class="widget-properties-panel-details" style="display:none">
                    <table width="100%">
                        <tr><td>
                            <div class="input-group">
                                <span class="input-group-addon" id="sizing-addon1">Signal</span>
                                <select id="select_signal" class="form-control widget-properties-panel-input" aria-describedby="sizing-addon1">
                                </select>
                            </div>
                            </td>
                            <td>&nbsp;</td>
                            <td width="60px">
                            <button id = "btn_signal_search" type="button" class="btn btn-default" aria-label="Left Align">
                                <span class="glyphicon glyphicon-search" aria-hidden="true"></span>
                            </button>
                            </td>
                        </tr>
                    </table>
                    <p></p>
                    <div class="input-group" style="display:none;">
                        <span class="input-group-addon" id="sizing-addon1">What</span>
                        <select id="select_sig_subtype" class="form-control widget-properties-panel-input" aria-describedby="sizing-addon1">
                            <option>Toggle</option>
                            <option>Watch Only</option>
                            <option>Plot</option>
                        </select>
                    </div>
                    <p></p>
                    <div class="panel-group" id="accordion_sel_1" role="tablist" aria-multiselectable="true">
                        <div class="panel panel-primary">
                            <div class="panel-heading" role="tab" id="accordion_sel_1_1_heading">
                                <h6 class="panel-title">
                                    <a role="button" data-toggle="collapse" data-parent="#accordion_sel_1" href="#accordion_sel_1_1_collapse" aria-expanded="true" aria-controls="accordion_sel_1_1_collapse">
                                        Toggle
                                    </a>
                                </h6>
                            </div>
                            <div id="accordion_sel_1_1_collapse" class="panel-collapse collapse" role="tabpanel" aria-labelledby="accordion_sel_1_1_heading">
                                <div class="panel-body">
                                    <div class="input-group">
                                        <span class="input-group-addon" id="sizing-addon1">Display Format</span>
                                        <select id="select_display_format" class="form-control widget-properties-panel-input" aria-describedby="sizing-addon1" disabled>
                                            <option>Decimal</option>
                                            <option>Hexadecimal</option>
                                            <option>Binary</option>
                                            <option>Octal</option>
                                        </select>
                                    </div>
                                    <p></p>
                                    <div class="input-group">
                                        <span class="input-group-addon" id="sizing-addon1">Update Value</span>
                                        <select id="select_update_behaviour" class="form-control widget-properties-panel-input" aria-describedby="sizing-addon1">
                                            <option selected>Automatically</option>
                                            <option>On Trigger</option>
                                            <option>Watch Only</option>
                                        </select>
                                    </div>                                    
                                </div>
                            </div>
                        </div>

                        <div class="panel panel-primary">
                            <div class="panel-heading" role="tab" id="accordion_sel_1_2_heading">
                                <h4 class="panel-title">
                                    <a class="collapsed" role="button" data-toggle="collapse" data-parent="#accordion_sel_1" href="#accordion_sel_1_2_collapse" aria-expanded="false" aria-controls="accordion_sel_1_2_collapse">
                                        Watch
                                    </a>
                                </h4>
                            </div>
                            <div id="accordion_sel_1_2_collapse" class="panel-collapse collapse" role="tabpanel" aria-labelledby="accordion_sel_1_2_heading">
                                <div class="panel-body">
                                    <div class="input-group">
                                        <span class="input-group-addon" id="sizing-addon1">Display Format</span>
                                        <select id="select_display_format_watch_only" class="form-control widget-properties-panel-input" aria-describedby="sizing-addon1" disabled>
                                            <option>Decimal</option>
                                            <option>Hexadecimal</option>
                                            <option>Binary</option>
                                            <option>Octal</option>
                                        </select>
                                    </div>
                                </div>
                            </div>
                        </div>

                        <div class="panel panel-primary">
                            <div class="panel-heading" role="tab" id="accordion_sel_1_3_heading">
                                <h4 class="panel-title">
                                    <a class="collapsed" role="button" data-toggle="collapse" data-parent="#accordion_sel_1" href="#accordion_sel_1_3_collapse" aria-expanded="false" aria-controls="accordion_sel_1_3_collapse">
                                        Plot
                                    </a>
                                </h4>
                            </div>
                            <div id="accordion_sel_1_3_collapse" class="panel-collapse collapse" role="tabpanel" aria-labelledby="accordion_sel_1_3_heading">
                                <div class="panel-body">
                                    &nbsp;
                                </div>
                            </div>
                        </div>
                    </div>
                </div>            
                <div id="details_for_sel_2" style="display:none" class="widget-properties-panel-details">
                   <table width="100%">
                        <tr><td>
                            <div class="input-group">
                                <span class="input-group-addon" id="sizing-addon1">Event</span>
                                <select id="select_event" class="form-control widget-properties-panel-input" aria-describedby="sizing-addon1">
                                </select>
                            </div>
                            </td>
                            <td>&nbsp;</td>
                            <td width="60px">
                            <button id = "btn_search_event" type="button" class="btn btn-default" aria-label="Left Align">
                                <span class="glyphicon glyphicon-search" aria-hidden="true"></span>
                            </button>
                            </td>
                        </tr>
                   </table>
                    <p></p>
                </div>
                <div id="details_for_sel_3" style="display:none" class="widget-properties-panel-details">
                </div>

            </div>
    </div>
 `;

function close_widget_properties_panel() {
    $("#widget_properties_panel").css("display","none");
    $(".baseWidget-selected").removeClass("baseWidget-selected");
}

function set_widget_panel_accordion_active(which) {
    if (which == widget_panel_accordion_active) return;
    let l = ["accordion_sel_1_1_collapse", "accordion_sel_1_2_collapse", "accordion_sel_1_3_collapse"];
    $("#" + l[which]).collapse("show");
    $("#" + l[widget_panel_accordion_active]).collapse("hide");

    widget_panel_accordion_active_last = widget_panel_accordion_active;
    widget_panel_accordion_active = which;
}


function init_widget_properties_panel() {
            console.log("init_widget_properties_panel()");
            $("body").append(widget_panel_html);
            prop_panel = $('#widget_properties_panel'); 

            $(prop_panel).find("#btn_signal_search").on('click', (ev) => {
                 $('#dlg_select_a_signal').modal('show');
              if (signals_dt == undefined) {
               dlg_select_a_signal_table_dataset = dlg_select_a_signal_populate_dataset();
               signals_dt = $('#dlg_select_a_signal_table').DataTable({
                  select: {
                       style: 'single'
                  },
                  data: dlg_select_a_signal_table_dataset,
                  columns: dlg_select_a_signal_table_columns
                 });
                }
            });

            $(prop_panel).find("#btn_search_event").on('click', (ev) => {
                $('#dlg_select_an_event').modal('show');
                if (simcore_events_dt == undefined) {
                    dlg_select_an_event_table_dataset = dlg_select_an_event_populate_dataset();
                    simcore_events_dt = $('#dlg_select_an_event_table').DataTable({
                        select: {
                            style: 'single'
                        },
                        data: dlg_select_an_event_table_dataset,
                        columns: dlg_select_an_event_table_columns
                    });
                }
            });

            $(".widget-properties-panel-input").change(() => {
              on_prop_panel_change();
            });
            prop_panel_header = $('#widget_properties_panel_header');
            prop_panel_header.mousedown((ev) => {
            let p = $('#widget_properties_panel_header').offset();
                let dx = ev.pageX - p.left;
                let dy = ev.pageY - p.top;

                $(document).mousemove((ev) => {
                  $('#widget_properties_panel').offset({ top: ev.pageY - dy, left: ev.pageX - dx });
                });
                $(document).mouseup((ev) => {
                $(document).unbind("mouseup").unbind("mousemove");
                });
            });

            setInterval(() => {
                function get_accordion_status(l) {
                    let r = [];
                    for (let e of l) {
                        let cl = $('#' + e).attr('class').split(/\s+/);
                        let open = false;
                        for (let ee of cl) { if (ee == "in") { open = true; break; } }
                        r.push(open);
                    }
                    return r;
                }
                let t = get_accordion_status(["accordion_sel_1_1_collapse", "accordion_sel_1_2_collapse", "accordion_sel_1_3_collapse"]);
                if (t[0]) widget_panel_accordion_active = ACC_TAB_TOGGLE_SIGNAL;
                else if (t[1]) widget_panel_accordion_active = ACC_TAB_WATCH_SIGNAL;
                else if (t[2]) widget_panel_accordion_active = ACC_TAB_PLOT;
                if (widget_panel_accordion_active_last != widget_panel_accordion_active) {
                    widget_panel_accordion_active_last = widget_panel_accordion_active;
                    on_prop_panel_change();
                }
            }, 100);
        }

        function handle_signal_select(sig_name) {
            $('#dlg_select_a_signal').modal('hide');
            let widget = $(".baseWidget-selected").first();
            let info = widget_properties_panel_extract_settings();
            info.sig_name = sig_name;
            save_widget_info(widget, info);
            update_prop_panel(widget);
        }
        function handle_event_select(ev_name) {
            $('#dlg_select_an_event').modal('hide');
            let widget = $(".baseWidget-selected").first();
            let info = widget_properties_panel_extract_settings();
            info.ev_name = ev_name;
            save_widget_info(widget, info);
            update_prop_panel(widget);
        }
        function get_widget_info(widget) {
            if (widget === undefined) return {valid: false };

            if (typeof widget == "string") {
            let widget_id = widget;
                for (let e of widget_infos) {
                    if (e.id == widget_id) {
                        return e.info;
                    }
                }
                return {valid: false };
            }
            let widget_id = widget.attr("id");
            for (let e of widget_infos) {
                if (e.id == widget_id) {
                    return e.info;
                }
            }

            if (!widget.hasClass("baseWidget")) return { valid: false };
            if (widget.hasClass("baseWidget-not-initialized")) return {
                valid: false,
                type: {sig: false, ev: false, script: false },
                doc: undefined,
                sig_name: undefined,
                ev_name: undefined,
                do_plot: false,
                watch_only: false,
                toggle_sig : false,
                update_sig_automatically: false,
                display : "Decimal"
            };
            return {valid: false };
        }

        function save_widget_info(widget, info, update_ctrl) {
            info.valid = info.type.sig || info.type.ev || info.type.script;

            let widget_id = undefined;
            if (typeof widget == "string") widget_id = widget;
            else widget_id = widget.attr("id");

            for (let e of widget_infos) {
                if (e.id == widget_id) {
                  e.info = info;
                  if (update_ctrl == undefined || update_ctrl) update_widget_ctrl(widget, info);
                  return true;
                }
            }
            widget_infos.push({id: widget_id, info: info });
            if (update_ctrl == undefined || update_ctrl) update_widget_ctrl(widget, info);
            return true;
        }

        function save_widget_ext(widget_id, ext) {
            for (let e of widget_infos) {
                if (e.id == widget_id) {
            e.ext = ext;
        return;
                }
            }
            throw "Internal Error:" + widget_id;
        }

        function update_widget_ctrl(widget, info) {
            let widget_id = (typeof widget == "string") ? widget : widget.attr("id");
            if (info == undefined)
                info = get_widget_info(widget);
            if (info.type.sig && info.sig_name != undefined) {
                if (info.toggle_sig) {
                    setup_toggle_widget(widget_id, get_signal_by_name(info.sig_name));
                } else if (info.watch_only) {
                    setup_toggle_widget(widget_id, get_signal_by_name(info.sig_name), true);
                } else if (info.do_plot) {
                    setup_plot_signal_widget(widget_id, get_signal_by_name(info.sig_name));
                }
            } else if (info.type.ev) {
                setup_trigger_event_widget(widget_id, info);
            }
        }

        function widget_properties_panel_extract_settings() {
           
            let type_ctrl = $(prop_panel).find("#select_widget_type");
            if (type_ctrl.length != 1) throw "Internal Error";
            let type = type_ctrl.find(":selected").text();
            if (type == "Modify/View Signal") {
                let sig_name = $(prop_panel).find("#select_signal").find(":selected").text();
                if (sig_name != undefined && sig_name.length == 0) sig_name = undefined;
                let sub_type = widget_panel_accordion_active;


                let update_behaviour = $(prop_panel).find("#select_update_behaviour").find(":selected").text();
                let display_format = $(prop_panel).find("#select_display_format").find(":selected").text();

                return {
                    valid: true,
                    type: {sig: true, ev: false, script: false },
                    doc: undefined,
                    sig_name: sig_name,
                    ev_name: undefined,
                    do_plot: sub_type == ACC_TAB_PLOT,
                    watch_only: sub_type == ACC_TAB_WATCH_SIGNAL,
                    toggle_sig: sub_type == ACC_TAB_TOGGLE_SIGNAL,
                    update_sig_automatically: update_behaviour == "Automatically",
                    display: display_format
                };
            } else if (type == "Trigger Event") {
                let ev_name = $(prop_panel).find("#select_event").find(":selected").text();
                if ((ev_name == undefined || ev_name == "") && simcore_events.length) ev_name = simcore_events[0].name;

                return {
                    valid: true,
                    type: {sig: false, ev: true, script: false },
                    doc: undefined,
                    sig_name: undefined,
                    ev_name: ev_name,
                    do_plot: false,
                    watch_only: false,
                    toggle_sig: false,
                    update_sig_automatically: false
                };
            } else if (type == "Run Script") {
                return {
            valid: true,
                    type: {sig: false, ev: false, script: true },
                    doc: undefined,
                    sig_name: undefined,
                    ev_name: undefined,
                    do_plot: false,
                    watch_only: false,
                    toggle_sig: false,
                    update_sig_automatically: false
                };
            }

            return {
            valid: false,
                type: {sig: false, ev: false, script: false },
                doc: undefined,
                sig_name: undefined,
                ev_name: undefined,
                do_plot: false,
                watch_only: false,
                toggle_sig: false,
                update_sig_automatically: false
            };
        }

        function on_prop_panel_change() {
            let widget = $(".baseWidget-selected").first();
            let info = widget_properties_panel_extract_settings();
            save_widget_info(widget, info);
            update_prop_panel(widget);
        }

        function helper_build_sel_html(list, selected) {
            let r = "";
            for (let i = 0; i != list.length; ++i) {
                if (i != selected) r += "<option value=\"" + list[i] + "\" >" + list[i] + "</option>";
                else r += "<option value=\"" + list[i] + "\" selected >" + list[i] + "</option>";
            }
return r;
        }

function update_prop_panel(widget) {

    let info = get_widget_info(widget);
    let update_widget = false;
    let widget_id = widget.attr("id");
    $(prop_panel).css("display", "block");
    if (!info.valid) {
        $(prop_panel).find(".widget-properties-panel-details").css("display", "none");
        update_widget = true;
    } else {
        if (info.sig_name != undefined) {
            let s = get_signal_by_name(info.sig_name);
            if (s != undefined && s.info.type == "int") {
                $("#select_display_format").removeAttr("disabled"); $("#select_display_format").removeAttr("enabled");
                $("#select_display_format").attr("enabled", "true");
            } else {
                $("#select_display_format").removeAttr("disabled"); $("#select_display_format").removeAttr("enabled");
                $("#select_display_format").attr("disabled", "true");
            }
        }
    }

    $(prop_panel).find("#select_widget_type").removeAttr('disabled');
    {
        let widget_type_sel = $(prop_panel).find("#select_widget_type");
        let i = 0;
        if (info.type.sig) i = 1;
        else if (info.type.ev) i = 2;
        else if (info.type.script) i = 3;
        widget_type_sel.html(helper_build_sel_html(["Choose ...", "Modify/View Signal", "Trigger Event"], i));
    }

    $(prop_panel).children(".panel-body").first().children("#details_for_sel_1").css("display", "none");
    $(prop_panel).children(".panel-body").first().children("#details_for_sel_2").css("display", "none");
    $(prop_panel).children(".panel-body").first().children("#details_for_sel_3").css("display", "none");

    if (info.type.sig) $(prop_panel).children(".panel-body").first().children("#details_for_sel_1").css("display", "block");
    if (info.type.ev) $(prop_panel).children(".panel-body").first().children("#details_for_sel_2").css("display", "block");
    if (info.type.script) $(prop_panel).children(".panel-body").first().children("#details_for_sel_3").css("display", "block");

    if (info.type.ev) {
        if (simcore_events.length == 0) {
            $("#select_event").prop("disabled", "true"); $("#btn_search_event").prop("disabled", "true");
        } else {
            let select_event_html = "";
            simcore_events.forEach((e) => { select_event_html += "<option value=\"" + e.name + "\" " + (e.name == info.ev_name ? " selected >" : ">") + e.name + "</option>"; });
            $("#select_event").html(select_event_html);
        }
    }

    if (info.type.sig) {
        let sig_sel = $(prop_panel).children(".panel-body").first().children("#details_for_sel_1").first().find("#select_signal").first();
        let sig_sel_html = "";
        signals.forEach((e) => { sig_sel_html += "<option value=\"" + e.name + "\" " + (e.name == info.sig_name ? " selected >" : ">") + e.name + "</option>"; });
        sig_sel.html(sig_sel_html);        
        {
            if (!update_widget) {
                if (info.toggle_sig) set_widget_panel_accordion_active(ACC_TAB_TOGGLE_SIGNAL);
                else if (info.watch_only) set_widget_panel_accordion_active(ACC_TAB_WATCH_SIGNAL);
                else if (info.do_plot) set_widget_panel_accordion_active(ACC_TAB_PLOT);
            }            
        }

        {
            let update_behaviour_sel = $(prop_panel).find("#select_update_behaviour");
            let i = 0;
            if (!update_widget) if (info.update_sig_automatically) i = 0; else i = 1;
            update_behaviour_sel.html((helper_build_sel_html(["Automatically", "On Trigger", "Watch Only"], i)));
        }

        {
            let display_format_sel = $(prop_panel).find("#select_display_format");
            let i = 0;
            if (info.display == "Hexadecimal") i = 1;
            else if (info.display == "Binary") i = 2;
            else if (info.display == "Octal") i = 3;
            display_format_sel.html((helper_build_sel_html(["Decimal", "Hexadecimal", "Binary", "Octal"], i)));
        }
    }

    if (!update_widget) {
        if (info.type.sig && info.update_sig_automatically) $(prop_panel).find("#btn_update_signal_value").removeAttr("disabled");
        else if (info.type.sig && !info.update_sig_automatically) $(prop_panel).find("#btn_update_signal_value").removeAttr("disabled").attr("disabled", "");
    }

    if (update_widget) {
        let new_info = widget_properties_panel_extract_settings();
        if (new_info.valid) widget.removeClass("baseWidget-not-initialized");
        save_widget_info(widget, new_info);
    }
}
   