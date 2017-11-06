


function setup_toggle_widget(widget_id, signal, watch) {
    if (watch == undefined) watch = false;

    $(`#header_of_${widget_id}`).html(
        signal.name +
        `<button type="button" class="btn btn-xs close" aria-label="Close" onclick="close_widget(event,'${widget_id}');" style="transform: translateY(-11px);">
                    <span aria-hidden="true" class="small" >&times;</span>
                </button>`
    );
    $(`#${widget_id}`).removeClass("toggle-signal").addClass("toggle-signal");
    $(`#${widget_id}`).removeClass("depend-on-signal-data").addClass("depend-on-signal-data");
    $(`#${widget_id}`).removeClass("baseWidget-not-initialized");
    $(`#${widget_id}`).removeClass("waiting-for-data");

    let widget_content = helper_make_toggle_widget_content(widget_id, signal,watch);
 
    $(`#content_of_${widget_id}`).html(widget_content);

    if (signal.value == undefined) {
        $(`#${widget_id}`).addClass("waiting-for-data");
        $(`#${widget_id}`).attr("attached-signal",signal.name);
        return;
    }

    if (signal.comment != undefined) {
        $(`#header_of_${widget_id}`).html(limit_to_n_characters(signal.name + "( " + signal.comment + " )", 40) + `<button type="button" class="btn btn-xs close" aria-label="Close" onclick="close_widget(event,'${widget_id}');" style="transform: translateY(-11px);">
                    <span aria-hidden="true" class="small" >&times;</span>
                </button>`);
    }


    let gran = slider_granularity_default - 1;
    if (signal.info.type == "int") gran = Math.min(slider_granularity_default - 1, (signal.info.max - signal.info.min));

    function on_thumb_moved (ev, ui) {
        let v = map_range_index_to_value(signal, gran, ui.value);
        let v_text = sig_value_text_representation(signal, v, get_widget_info(widget_id).display);
        $("#" + widget_id + "_text_ctrl").val(v_text);
        set_signal_target_value(widget_id, v, signal.name);
        if ($("#" + widget_id + "_input_group").hasClass("has-error")) {
            $("#" + widget_id + "_text_ctrl").popover('destroy');
            $("#" + widget_id + "_input_group").removeClass("has-error");
        }

        if (signal.info.vm != undefined && signal.info.vm.length > 0) {
            let [l, sel_index] = helper_make_toggle_widget_sel_list(signal, signal.target_value);
            $(`#${widget_id}_select_ctrl`).html(helper_build_sel_html(l, sel_index));
        }
    };
    let s = undefined;
    let popover_active = false;

    function check_validity(v) {
        let r = check_constraints(signal, v);
        if (!r[0]) {
            $("#" + widget_id + "_text_ctrl").attr("data-content", r[1]);
            if (!popover_active) {
                $("#" + widget_id + "_input_group").addClass("has-error");
                $("#" + widget_id + "_text_ctrl").popover({ placement: 'auto top' });
                $("#" + widget_id + "_text_ctrl").attr("title", "");
                $("#" + widget_id + "_text_ctrl").popover('show');
            }
            popover_active = true;
        } else {
            if (popover_active) {
                $("#" + widget_id + "_text_ctrl").popover('destroy');
                $("#" + widget_id + "_input_group").removeClass("has-error");
            }
            popover_active = false;
        }
    };
    //console.log(Math.round(map_sig_target_value_to_range(signal, slider_granularity_default - 1)));
    let min_is_max = false;
    let min_is_bigger_max = false;

    if (signal.info.min != undefined && signal.info.max != undefined)
      if (signal.info.min == signal.info.max) min_is_max = true;
        else if (signal.info.min > signal.info.max) min_is_bigger_max = true;

    if (min_is_max) warn(signal.name + ": Minimum equals Maximum in constraints definition of this signal!");
    if (min_is_bigger_max) fatal(signal.name + ": Minimum strictly greater than Maximum in constraints definition of this signal!");

    if (signal.info.type == "float") {
        s = $("#dummy_" + widget_id).slider(
            {
                min: 0,
                max: min_is_max ? 0 : slider_granularity_default - 1,
                value:  Math.round(map_sig_target_value_to_range(signal, slider_granularity_default - 1)),
                slide: on_thumb_moved,
                disabled: watch
            });
        save_widget_ext(widget_id, {
            slider: s,
            slider_range: gran,
            
            set_value: function (v) {
                let v_text = sig_value_text_representation(signal, v, get_widget_info(widget_id).display);
                $("#" + widget_id + "_text_ctrl").val(v_text);
                let vv = map_sig_value_to_range(signal, gran);
                $(s).slider("value", vv);
                check_validity(v);
            }
        });
    } else if (signal.info.type == "int") {
                
        s = $("#dummy_" + widget_id).slider(
            {
                min: 0,
                max: gran,
                value: map_sig_target_value_to_range(signal, gran),
                slide: on_thumb_moved,
                disabled: watch
            });
        $(`#${widget_id}_select_ctrl`).change(function (ev) {
            let v_str = $(`#${widget_id}_select_ctrl`).find(":selected").text();
            let [l, old_idx] = helper_make_toggle_widget_sel_list(signal, signal.target_value);
            let new_idx = 0;
            for (let i = 0; i != signal.info.vm.length; ++i) if (signal.info.vm[i].name == v_str) { new_idx = i + 1; break; }
            if (new_idx == 0 && old_idx != 0) {
                let s = signal.info.vm[old_idx - 1].name;
                $(`#${widget_id}_select_ctrl option[value='${s}']`).prop('selected', true);
            } else if (new_idx != 0) {
                set_signal_target_value(widget_id, signal.info.vm[new_idx - 1].val, signal.name);
                let v_text = sig_value_text_representation(signal, signal.target_value, get_widget_info(widget_id).display);
                $("#" + widget_id + "_text_ctrl").val(v_text);
                let vv = map_sig_target_value_to_range(signal, gran);
                $(s).slider("value", vv);
            }
        });
        save_widget_ext(widget_id, {
            slider: s,
            slider_range: gran,
            set_value: function (v) {
                let v_text = sig_value_text_representation(signal, v, get_widget_info(widget_id).display);
                $("#" + widget_id + "_text_ctrl").val(v_text);
                if (signal.info.vm != undefined && signal.info.vm.length > 0) {
                    let [l, sel_index] = helper_make_toggle_widget_sel_list(signal, signal.target_value);
                    $(`#${widget_id}_select_ctrl`).html(helper_build_sel_html(l, sel_index));
                }
                let vv = map_sig_target_value_to_range(signal, gran);
                $(s).slider("value", vv);
                check_validity(v);
                /*let r = check_constraints(signal, v);
                if (!r[0]) {
                    $("#" + widget_id + "_input_group").addClass("has-error");
                    $("#" + widget_id + "_text_ctrl").popover({ placement: 'auto right' });
                    $("#" + widget_id + "_text_ctrl").attr("title", "");
                    $("#" + widget_id + "_text_ctrl").attr("data-content", r[1]);
                    $("#" + widget_id + "_text_ctrl").popover('show');
                } else $("#" + widget_id + "_text_ctrl").popover('destroy');*/
            }
        });
    }
    $(`#${widget_id}_text_ctrl`).on('input', function (ev) {
        let [ok, v, msg] = validate_input(signal, $(`#${widget_id}_text_ctrl`).val(), get_widget_info(widget_id).display);
       
        $("#" + widget_id + "_input_group").removeClass("has-error");
        if (!ok) {
            $("#" + widget_id + "_input_group").addClass("has-error");
            $("#" + widget_id + "_text_ctrl").popover({ placement: 'auto right' });
            $("#" + widget_id + "_text_ctrl").attr("title", "");
            $("#" + widget_id + "_text_ctrl").attr("data-content", msg);
            $("#" + widget_id + "_text_ctrl").popover('show');
        } else {
            $("#" + widget_id + "_text_ctrl").popover('destroy');
            set_signal_target_value(widget_id, v, signal.name);
            let vv = map_sig_target_value_to_range(signal, gran);
            $(s).slider("value", vv);
            if (signal.info.vm != undefined && signal.info.vm.length > 0) {
                let [l, sel_index] = helper_make_toggle_widget_sel_list(signal, signal.target_value);
                $(`#${widget_id}_select_ctrl`).html(helper_build_sel_html(l, sel_index));
            }
        }
    });

}

function setup_trigger_event_widget(widget_id, info) {
    let event_name = info.ev_name;
    $(`#header_of_${widget_id}`).html(
        event_name +
        `<button type="button" class="btn btn-xs close" aria-label="Close" onclick="close_widget(event,'${widget_id}');" style="transform: translateY(-11px);">
                    <span aria-hidden="true" class="small" >&times;</span>
                </button>`
    );
    $(`#${widget_id}`).removeClass("trigger-event").removeClass("toggle-signal").addClass("toggle-signal");
    $(`#${widget_id}`).removeClass("depend-on-event").removeClass("depend-on-signal-data").addClass("depend-on-event");
    $(`#${widget_id}`).removeClass("baseWidget-not-initialized");
    $(`#${widget_id}`).removeClass("waiting-for-data");

    let widget_content = helper_make_trigger_event_widget_content(widget_id, info, limit_to_n_characters(event_name, 30));
    //console.log(widget_content);

    $(`#content_of_${widget_id}`).html(widget_content);

    $(`#${widget_id}_trigger_ev_btn`).on("click", (ev) => {
        let ws = get_simcore_ws_given_widget_id(widget_id);
        ws.send(`EVENT ${event_name}`);
    });   
}
