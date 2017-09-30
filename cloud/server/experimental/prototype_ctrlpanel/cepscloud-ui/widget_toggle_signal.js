function setup_toggle_widget(widget_id, signal) {
    $(`#header_of_${widget_id}`).html(
        signal.name +
        `<button type="button" class="btn btn-xs close" aria-label="Close" onclick="close_widget(event,'${widget_id}');" style="transform: translateY(-11px);">
                    <span aria-hidden="true" class="small" >&times;</span>
                </button>`
    );
    $(`#${widget_id}`).addClass("toggle-signal");
    $(`#${widget_id}`).removeClass("baseWidget-not-initialized");
    let widget_content = helper_make_toggle_widget_content(widget_id, signal);

    $(`#content_of_${widget_id}`).html(widget_content);
    if (signal.info.type == "float") {
        let s = $("#dummy_" + widget_id).slider(
            {
                min: 0,
                max: slider_granularity_default - 1,
                value: map_sig_target_value_to_range(signal, slider_granularity_default - 1),
                slide: function (ev, ui) {
                    let v = map_range_index_to_value(signal, slider_granularity_default - 1, ui.value);
                    $("#" + widget_id + "_text_ctrl").val(v.toString());
                    set_signal_target_value(widget_id, v, signal.name);
                }
            });
        save_widget_ext(widget_id, {
            slider: s,
            slider_range: slider_granularity_default - 1,
            set_value: function (v) {
                $("#" + widget_id + "_text_ctrl").val(v.toString());
                let vv = map_sig_target_value_to_range(signal, slider_granularity_default - 1);
                $(s).slider("value", vv);
            }
        });
    } else if (signal.info.type == "int") {
        let gran = Math.min(slider_granularity_default - 1, (signal.info.max - signal.info.min));
        let s = $("#dummy_" + widget_id).slider(
            {
                min: 0,
                max: gran,
                value: map_sig_target_value_to_range(signal, gran),
                slide: function (ev, ui) {
                    let v = map_range_index_to_value(signal, gran, ui.value);
                    let v_text = sig_value_text_representation(signal, v, get_widget_info(widget_id).display);
                    $("#" + widget_id + "_text_ctrl").val(v_text);
                    set_signal_target_value(widget_id, v, signal.name);

                    if (signal.info.vm != undefined && signal.info.vm.length > 0) {
                        let [l, sel_index] = helper_make_toggle_widget_sel_list(signal, signal.target_value);
                        $(`#${widget_id}_select_ctrl`).html(helper_build_sel_html(l, sel_index));
                    }
                }
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
        $(`#${widget_id}_text_ctrl`).on('input', function (ev) {
            let [ok, v, msg] = validate_input(signal, $(`#${widget_id}_text_ctrl`).val(), get_widget_info(widget_id).display);
            $("#" + widget_id + "_input_group").removeClass("has-error");
            if (!ok) {
                $("#" + widget_id + "_input_group").addClass("has-error");
                $("#" + widget_id + "_text_ctrl").popover({ placement: 'left' });
                $("#" + widget_id + "_text_ctrl").attr("title", "1111111");
                $("#" + widget_id + "_text_ctrl").attr("data-content", msg);
                $("#" + widget_id + "_text_ctrl").popover('show');
            } else {
                $("#" + widget_id + "_text_ctrl").popover('hide');
                set_signal_target_value(widget_id, v, signal.name);
                let vv = map_sig_target_value_to_range(signal, gran);
                $(s).slider("value", vv);
                if (signal.info.vm != undefined && signal.info.vm.length > 0) {
                    let [l, sel_index] = helper_make_toggle_widget_sel_list(signal, signal.target_value);
                    $(`#${widget_id}_select_ctrl`).html(helper_build_sel_html(l, sel_index));
                }
            }
        });
        save_widget_ext(widget_id, {
            slider: s,
            slider_range: slider_granularity_default - 1,
            set_value: function (v) {
                let v_text = sig_value_text_representation(signal, v, get_widget_info(widget_id).display);
                $("#" + widget_id + "_text_ctrl").val(v_text);
                if (signal.info.vm != undefined && signal.info.vm.length > 0) {
                    let [l, sel_index] = helper_make_toggle_widget_sel_list(signal, signal.target_value);
                    $(`#${widget_id}_select_ctrl`).html(helper_build_sel_html(l, sel_index));
                }
                let vv = map_sig_target_value_to_range(signal, gran);
                $(s).slider("value", vv);
            }
        });
    }
}
