function build_ctrlpanel_from_json(container, descr) {
    let t = 50;
    for (let e of descr.content) {
        let info = ctrlpanel_info_get_widget_descr(e);
        
        if (!info[0]) continue;
        let widget = container.addWidget();

        let widget_info = {
            valid: true,
            type: { sig: info[1].type == "toggle_signal" || info[1].type == "watch_signal" || info[1].type == "plot_signal", ev: false, script: false },
            doc: undefined,
            sig_name: info[1].associated_systemstate,
            ev_name: undefined,
            do_plot: info[1].type == "plot_signal",
            watch_only: info[1].type == "watch_signal",
            toggle_sig: info[1].type == "toggle_signal",
            update_sig_automatically: false,
            display: "Decimal"
        };

        //console.log(widget_info);

        if (info[1].style != undefined && info[1].style.width != undefined) $("#" + widget).css("width", info[1].style.width);
        if (info[1].style != undefined && info[1].style.height != undefined) $("#" + widget).css("height", info[1].style.height);

        setTimeout(() => { save_widget_info(widget, widget_info, true); }, t);
        t += 50;
        //save_widget_info(widget, widget_info, true);
    }
}